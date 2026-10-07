#include "platform/teensy/lights/TeensyLights.h"

#include <Arduino.h>
#include <OctoWS2811.h>

#include <algorithm>
#include <cstdint>

#ifndef BLASTER_TEENSY_LED_COUNT
#define BLASTER_TEENSY_LED_COUNT 1
#endif

#ifndef BLASTER_TEENSY_LED_PIN
#define BLASTER_TEENSY_LED_PIN 2
#endif

static_assert(BLASTER_TEENSY_LED_COUNT > 0, "BLASTER_TEENSY_LED_COUNT must be greater than zero");

namespace {
constexpr int kBytesPerLed = 3;
constexpr int kLedPinCount = 1;
constexpr int kLedBufferWords = (BLASTER_TEENSY_LED_COUNT * kLedPinCount * kBytesPerLed + 3) / 4;
constexpr int kLedConfig = WS2811_GRB | WS2811_800kHz;
constexpr int kDefaultBrightness = 255;
constexpr int kDefaultFlashDurationMs = 60;
constexpr int kDefaultFlashCount = 1;
constexpr int kDefaultFlashGapMs = 40;

DMAMEM int displayMemory[kLedBufferWords];
int drawingMemory[kLedBufferWords];
byte ledPins[kLedPinCount] = {BLASTER_TEENSY_LED_PIN};

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
}

TeensyLights::TeensyLights() = default;

TeensyLights::~TeensyLights() {
    delete m_leds;
    m_leds = nullptr;
}

bool TeensyLights::begin() {
    if (m_leds != nullptr) {
        return true;
    }

    m_leds = new OctoWS2811(
        BLASTER_TEENSY_LED_COUNT,
        displayMemory,
        drawingMemory,
        kLedConfig,
        kLedPinCount,
        ledPins);

    if (m_leds == nullptr) {
        return false;
    }

    m_leds->begin();
    render(RenderState{});
    return true;
}

void TeensyLights::setPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_basePattern = ActivePattern{pattern, static_cast<std::uint32_t>(::millis())};
}

void TeensyLights::flashPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_overridePattern = ActivePattern{pattern, static_cast<std::uint32_t>(::millis())};
}

void TeensyLights::flash() {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Flash;
    pattern.color = {255, 255, 255};
    pattern.brightness = kDefaultBrightness;
    pattern.durationMs = kDefaultFlashDurationMs;
    pattern.count = kDefaultFlashCount;
    pattern.intervalMs = kDefaultFlashGapMs;
    flashPattern(pattern);
}

void TeensyLights::update() {
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

TeensyLights::RenderState TeensyLights::renderPattern(
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

            const std::uint32_t local = cycleMs == 0 ? 0 : elapsedMs % cycleMs;
            if (local >= onMs) {
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

int TeensyLights::clampedBrightness(const weapon_behavior::LightPatternDef& pattern) {
    return clampByte(pattern.brightness.value_or(kDefaultBrightness));
}

void TeensyLights::render(const RenderState& state) {
    if (m_leds == nullptr) {
        return;
    }

    const std::uint32_t color = scaleColor(state.color, state.brightness);
    if (color == m_lastColor) {
        return;
    }

    for (int index = 0; index < BLASTER_TEENSY_LED_COUNT; ++index) {
        m_leds->setPixel(index, color);
    }
    m_leds->show();
    m_lastColor = color;
}
