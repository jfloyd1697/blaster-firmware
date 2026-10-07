#include "platform/stm32f411/lights/Stm32F411Lights.h"

#include <Arduino.h>
#include <stm32f4xx_hal.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace {
constexpr std::uint32_t kWs2812FrequencyHz = 800000;
constexpr std::uint16_t kTimerTicksPerBit = 125;
constexpr std::uint16_t kZeroDutyTicks = 40;
constexpr std::uint16_t kOneDutyTicks = 80;
constexpr std::size_t kBitsPerLed = 24;
constexpr std::size_t kResetSlots = 64;
constexpr int kDefaultBrightness = 255;
constexpr int kDefaultFlashDurationMs = 60;
constexpr int kDefaultFlashCount = 1;
constexpr int kDefaultFlashGapMs = 40;

TIM_HandleTypeDef g_tim4{};
DMA_HandleTypeDef g_tim4Dma{};
Stm32F411Lights* g_activeLights = nullptr;

int clampByte(const int value) {
    return std::clamp(value, 0, 255);
}

std::uint32_t scaleColor(const std::array<int, 3>& color, const int brightness) {
    const int level = clampByte(brightness);
    const std::uint32_t red = static_cast<std::uint32_t>(clampByte(color[0]) * level / 255);
    const std::uint32_t green = static_cast<std::uint32_t>(clampByte(color[1]) * level / 255);
    const std::uint32_t blue = static_cast<std::uint32_t>(clampByte(color[2]) * level / 255);
    return (red << 16u) | (green << 8u) | blue;
}

void appendByte(std::vector<std::uint16_t>& data, const std::uint8_t value, std::size_t& offset) {
    for (int bit = 7; bit >= 0; --bit) {
        data[offset++] = ((value >> bit) & 1u) != 0u ? kOneDutyTicks : kZeroDutyTicks;
    }
}
}

extern "C" void DMA1_Stream0_IRQHandler() {
    HAL_DMA_IRQHandler(&g_tim4Dma);
}

extern "C" void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM4 && g_activeLights != nullptr) {
        g_activeLights->onDmaComplete();
    }
}

Stm32F411Lights::Stm32F411Lights(const Stm32F411LightPins& pins)
    : m_pins(pins) {
}

Stm32F411Lights::~Stm32F411Lights() {
    waitForTransfer();
    if (g_activeLights == this) {
        g_activeLights = nullptr;
    }
}

bool Stm32F411Lights::begin() {
    if (m_pins.ledCount == 0 || m_pins.data != PB6) {
        return false;
    }

    m_pwmData.resize(static_cast<std::size_t>(m_pins.ledCount) * kBitsPerLed + kResetSlots, 0);
    if (!initializeTimerDma()) {
        return false;
    }

    g_activeLights = this;
    render(RenderState{});
    return true;
}

void Stm32F411Lights::setPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_basePattern = ActivePattern{pattern, static_cast<std::uint32_t>(::millis())};
}

void Stm32F411Lights::flashPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_overridePattern = ActivePattern{pattern, static_cast<std::uint32_t>(::millis())};
}

void Stm32F411Lights::flash() {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Flash;
    pattern.color = {255, 255, 255};
    pattern.brightness = kDefaultBrightness;
    pattern.durationMs = kDefaultFlashDurationMs;
    pattern.count = kDefaultFlashCount;
    pattern.intervalMs = kDefaultFlashGapMs;
    flashPattern(pattern);
}

void Stm32F411Lights::update() {
    const auto now = static_cast<std::uint32_t>(::millis());
    if (m_overridePattern.has_value()) {
        const auto elapsed = now - m_overridePattern->startMs;
        const RenderState state = renderPattern(m_overridePattern->pattern, elapsed, false);
        if (!state.finished) {
            render(state);
            return;
        }
        m_overridePattern.reset();
    }

    if (m_basePattern.has_value()) {
        const auto elapsed = now - m_basePattern->startMs;
        render(renderPattern(m_basePattern->pattern, elapsed, true));
        return;
    }

    render(RenderState{});
}

Stm32F411Lights::RenderState Stm32F411Lights::renderPattern(
    const weapon_behavior::LightPatternDef& pattern,
    const std::uint32_t elapsedMs,
    const bool loop) {
    RenderState state;
    state.color = pattern.color;
    state.brightness = clampedBrightness(pattern);

    switch (pattern.mode) {
        case weapon_behavior::LightPatternMode::Solid:
            if (!loop && elapsedMs >= static_cast<std::uint32_t>(
                    std::max(pattern.durationMs.value_or(kDefaultFlashDurationMs), 1))) {
                state.brightness = 0;
                state.finished = true;
            }
            return state;

        case weapon_behavior::LightPatternMode::Flash: {
            const std::uint32_t onMs = static_cast<std::uint32_t>(std::max(pattern.durationMs.value_or(0), 1));
            const std::uint32_t offMs = static_cast<std::uint32_t>(std::max(pattern.intervalMs.value_or(0), 0));
            const std::uint32_t count = static_cast<std::uint32_t>(std::max(pattern.count.value_or(1), 1));
            const std::uint32_t cycleMs = onMs + offMs;
            const std::uint32_t totalMs = cycleMs * count;

            if (!loop && elapsedMs >= totalMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            if (elapsedMs % cycleMs >= onMs) {
                state.brightness = 0;
            }
            return state;
        }

        case weapon_behavior::LightPatternMode::Pulse: {
            const std::uint32_t intervalMs = static_cast<std::uint32_t>(std::max(pattern.intervalMs.value_or(1), 1));
            const std::uint32_t durationMs = static_cast<std::uint32_t>(
                std::max(pattern.durationMs.value_or(static_cast<int>(intervalMs)), 1));
            if (!loop && elapsedMs >= durationMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }
            const std::uint32_t phase = elapsedMs % intervalMs;
            const std::uint32_t half = std::max<std::uint32_t>(intervalMs / 2u, 1u);
            const std::uint32_t ramp = phase <= half ? phase : intervalMs - phase;
            state.brightness = state.brightness * static_cast<int>(ramp) / static_cast<int>(half);
            return state;
        }

        case weapon_behavior::LightPatternMode::Sequence: {
            if (pattern.steps.empty()) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            std::uint32_t totalMs = 0;
            for (const auto& step : pattern.steps) {
                totalMs += static_cast<std::uint32_t>(std::max(step.durationMs, 0));
            }

            if (totalMs == 0) {
                state.color = pattern.steps.back().color;
                return state;
            }

            if (!loop && elapsedMs >= totalMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            const std::uint32_t sequenceTime = loop ? elapsedMs % totalMs : elapsedMs;
            std::uint32_t stepStart = 0;
            for (const auto& step : pattern.steps) {
                const auto duration = static_cast<std::uint32_t>(std::max(step.durationMs, 0));
                if (sequenceTime < stepStart + duration) {
                    state.color = step.color;
                    return state;
                }
                stepStart += duration;
            }

            state.color = pattern.steps.back().color;
            return state;
        }
    }

    return state;
}

void Stm32F411Lights::onDmaComplete() {
    HAL_TIM_PWM_Stop_DMA(&g_tim4, TIM_CHANNEL_1);
    m_transferActive = false;
}

int Stm32F411Lights::clampedBrightness(const weapon_behavior::LightPatternDef& pattern) {
    return clampByte(pattern.brightness.value_or(kDefaultBrightness));
}

bool Stm32F411Lights::initializeTimerDma() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef gpio{};
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOB, &gpio);

    const std::uint32_t timerClockHz = HAL_RCC_GetPCLK1Freq() * 2u;
    const std::uint32_t targetTimerHz = kWs2812FrequencyHz * kTimerTicksPerBit;
    const std::uint32_t prescaler = timerClockHz / targetTimerHz;
    if (prescaler == 0 || timerClockHz / prescaler != targetTimerHz) {
        return false;
    }

    g_tim4.Instance = TIM4;
    g_tim4.Init.Prescaler = prescaler - 1u;
    g_tim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim4.Init.Period = kTimerTicksPerBit - 1u;
    g_tim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_tim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&g_tim4) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef channel{};
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&g_tim4, &channel, TIM_CHANNEL_1) != HAL_OK) {
        return false;
    }

    g_tim4Dma.Instance = DMA1_Stream0;
    g_tim4Dma.Init.Channel = DMA_CHANNEL_2;
    g_tim4Dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_tim4Dma.Init.PeriphInc = DMA_PINC_DISABLE;
    g_tim4Dma.Init.MemInc = DMA_MINC_ENABLE;
    g_tim4Dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_tim4Dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    g_tim4Dma.Init.Mode = DMA_NORMAL;
    g_tim4Dma.Init.Priority = DMA_PRIORITY_HIGH;
    g_tim4Dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&g_tim4Dma) != HAL_OK) {
        return false;
    }

    __HAL_LINKDMA(&g_tim4, hdma[TIM_DMA_ID_CC1], g_tim4Dma);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    return true;
}

void Stm32F411Lights::render(const RenderState& state) {
    const std::uint32_t color = scaleColor(state.color, state.brightness);
    if (color == m_lastColor || m_transferActive) {
        return;
    }

    transmitColor(color);
    m_lastColor = color;
}

void Stm32F411Lights::transmitColor(const std::uint32_t color) {
    const std::uint8_t red = static_cast<std::uint8_t>((color >> 16u) & 0xFFu);
    const std::uint8_t green = static_cast<std::uint8_t>((color >> 8u) & 0xFFu);
    const std::uint8_t blue = static_cast<std::uint8_t>(color & 0xFFu);

    std::size_t offset = 0;
    for (std::uint16_t led = 0; led < m_pins.ledCount; ++led) {
        appendByte(m_pwmData, green, offset);
        appendByte(m_pwmData, red, offset);
        appendByte(m_pwmData, blue, offset);
    }
    std::fill(m_pwmData.begin() + static_cast<std::ptrdiff_t>(offset), m_pwmData.end(), 0);

    m_transferActive = HAL_TIM_PWM_Start_DMA(
        &g_tim4,
        TIM_CHANNEL_1,
        reinterpret_cast<std::uint32_t*>(m_pwmData.data()),
        static_cast<std::uint16_t>(m_pwmData.size())) == HAL_OK;
}

void Stm32F411Lights::waitForTransfer() {
    const auto start = ::millis();
    while (m_transferActive && (::millis() - start) < 10u) {
        if (HAL_DMA_GetState(&g_tim4Dma) == HAL_DMA_STATE_READY) {
            m_transferActive = false;
        }
    }
}
