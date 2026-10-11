#include "platform/stm32f411/lights/Stm32F411LightBackend.h"

#include <Arduino.h>
#include <stm32f4xx_hal.h>

#include <algorithm>
#include <string>

#include "core/debug/IDebug.h"
#include <cstddef>
#include <cstdint>

namespace {
constexpr std::uint32_t kWs2812FrequencyHz = 800000;
// WS2812 timing is relative to the actual TIM4 clock, not a fixed 100 MHz bus.
constexpr std::uint32_t kMinTicksPerBit = 40;
constexpr std::uint32_t kMaxTicksPerBit = 250;
constexpr std::size_t kBitsPerLed = 24;
constexpr std::size_t kResetSlots = 64;

TIM_HandleTypeDef g_tim4{};
DMA_HandleTypeDef g_tim4Dma{};
Stm32F411LightBackend* g_activeLights = nullptr;

void appendByte(std::vector<std::uint16_t>& data, const std::uint8_t value,
                const std::uint16_t zeroDuty, const std::uint16_t oneDuty,
                std::size_t& offset) {
    for (int bit = 7; bit >= 0; --bit) {
        data[offset++] = ((value >> bit) & 1u) != 0u ? oneDuty : zeroDuty;
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

Stm32F411LightBackend::Stm32F411LightBackend(const Stm32F411LightPins& pins, IDebug* debug)
    : m_pins(pins), m_debug(debug) {
}

Stm32F411LightBackend::~Stm32F411LightBackend() {
    waitForTransfer();
    if (g_activeLights == this) {
        g_activeLights = nullptr;
    }
}

bool Stm32F411LightBackend::begin() {
    if (m_pins.ledCount == 0 || m_pins.data != PB6) {
        if (m_debug) m_debug->error("Stm32F411LightBackend: expected PB6 and at least one LED");
        return false;
    }
    m_pwmData.resize(static_cast<std::size_t>(m_pins.ledCount) * kBitsPerLed + kResetSlots, 0);
    if (!initializeTimerDma()) {
        return false;
    }
    g_activeLights = this;
    m_initialized = true;
    if (m_debug) m_debug->log("Stm32F411LightBackend: TIM4/DMA initialized for " +
                              std::to_string(m_pins.ledCount) + " LED(s)");
    return true;
}

std::size_t Stm32F411LightBackend::ledCount() const {
    return m_pins.ledCount;
}

void Stm32F411LightBackend::show(const std::vector<LightColor>& pixels) {
    if (!m_initialized || m_transferActive || pixels.size() != ledCount()) {
        return;
    }

    std::size_t offset = 0;
    for (const auto& color : pixels) {
        appendByte(m_pwmData, color[1], m_zeroDutyTicks, m_oneDutyTicks, offset);
        appendByte(m_pwmData, color[0], m_zeroDutyTicks, m_oneDutyTicks, offset);
        appendByte(m_pwmData, color[2], m_zeroDutyTicks, m_oneDutyTicks, offset);
    }
    std::fill(m_pwmData.begin() + static_cast<std::ptrdiff_t>(offset), m_pwmData.end(), 0);

    const HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(
        &g_tim4,
        TIM_CHANNEL_1,
        reinterpret_cast<std::uint32_t*>(m_pwmData.data()),
        static_cast<std::uint16_t>(m_pwmData.size()));
    m_transferActive = status == HAL_OK;
    if (status != HAL_OK && !m_reportedTransferError && m_debug) {
        m_reportedTransferError = true;
        m_debug->error("Stm32F411LightBackend: TIM4 PWM DMA start failed (HAL " +
                       std::to_string(static_cast<int>(status)) +
                       ", DMA state " + std::to_string(static_cast<int>(HAL_DMA_GetState(&g_tim4Dma))) + ")");
    }
}

void Stm32F411LightBackend::onDmaComplete() {
    HAL_TIM_PWM_Stop_DMA(&g_tim4, TIM_CHANNEL_1);
    m_transferActive = false;
}

bool Stm32F411LightBackend::initializeTimerDma() {
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

    // APB1 timers run at PCLK1 when prescaler is 1, otherwise at 2 x PCLK1.
    const bool apb1Divided = (RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1;
    const std::uint32_t timerClockHz = HAL_RCC_GetPCLK1Freq() * (apb1Divided ? 2u : 1u);
    const std::uint32_t ticksPerBit =
        (timerClockHz + kWs2812FrequencyHz / 2u) / kWs2812FrequencyHz;
    if (ticksPerBit < kMinTicksPerBit || ticksPerBit > kMaxTicksPerBit) {
        if (m_debug) m_debug->error("Stm32F411LightBackend: invalid TIM4 clock " +
                                   std::to_string(timerClockHz) + " Hz");
        return false;
    }
    const std::uint32_t actualBitRate = timerClockHz / ticksPerBit;
    // Reject invalid clocks outside the WS2812 bit period tolerance.
    if (actualBitRate < 760000u || actualBitRate > 840000u) {
        if (m_debug) m_debug->error("Stm32F411LightBackend: TIM4 cannot produce 800 kHz");
        return false;
    }
    m_zeroDutyTicks = static_cast<std::uint16_t>((ticksPerBit * 32u + 50u) / 100u);
    m_oneDutyTicks = static_cast<std::uint16_t>((ticksPerBit * 64u + 50u) / 100u);
    if (m_debug) m_debug->log("Stm32F411LightBackend: TIM4 " +
                              std::to_string(timerClockHz) + " Hz, PWM " +
                              std::to_string(actualBitRate) + " Hz, " +
                              std::to_string(ticksPerBit) + " ticks/bit");

    g_tim4.Instance = TIM4;
    g_tim4.Init.Prescaler = 0u;
    g_tim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim4.Init.Period = ticksPerBit - 1u;
    g_tim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_tim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&g_tim4) != HAL_OK) {
        if (m_debug) m_debug->error("Stm32F411LightBackend: HAL_TIM_PWM_Init failed");
        return false;
    }

    TIM_OC_InitTypeDef channel{};
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&g_tim4, &channel, TIM_CHANNEL_1) != HAL_OK) {
        if (m_debug) m_debug->error("Stm32F411LightBackend: TIM4 channel 1 configuration failed");
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
        if (m_debug) m_debug->error("Stm32F411LightBackend: DMA1 stream 0 initialization failed (error " +
                                   std::to_string(g_tim4Dma.ErrorCode) + ")");
        return false;
    }

    __HAL_LINKDMA(&g_tim4, hdma[TIM_DMA_ID_CC1], g_tim4Dma);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    return true;
}

void Stm32F411LightBackend::waitForTransfer() {
    const auto start = ::millis();
    while (m_transferActive && (::millis() - start) < 10u) {
        if (HAL_DMA_GetState(&g_tim4Dma) == HAL_DMA_STATE_READY) {
            m_transferActive = false;
        }
    }
}
