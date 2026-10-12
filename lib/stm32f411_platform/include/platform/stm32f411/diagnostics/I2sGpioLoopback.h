#ifndef BLASTER_STM32F411_I2S_GPIO_LOOPBACK_H
#define BLASTER_STM32F411_I2S_GPIO_LOOPBACK_H

#pragma once

#include <cstdint>

// Diagnostic-only frequency counter: TIM2 counts rising edges received on
// PA15 (TIM2_CH1). It does not use CPU-driven GPIO polling, which cannot
// accurately count the approximately 1.4 MHz I2S bit clock.
namespace stm32_i2s_loopback {

constexpr std::uint32_t kSampleRateHz = 44100u;
constexpr std::uint32_t kWordSelectHz = kSampleRateHz;
constexpr std::uint32_t kBitClockHz = kSampleRateHz * 16u * 2u;

bool begin();
void resetCounter();
// Drives 250 approximately 1 kHz GPIO pulses from PB8 while PA15 counts edges.
// Wire PB8 -> PA15 before running. This proves the capture path independently of I2S.
std::uint32_t runReferencePulseTest();
std::uint32_t takeRisingEdges();
void end();

// A tolerance for the STM32's actual PLL clock and the measurement interval.
// DATA is intentionally not classified as a valid I2S stream merely because
// it toggles: edge counting cannot check the serial word framing or samples.
inline bool nearFrequency(const std::uint32_t measuredHz, const std::uint32_t expectedHz) {
    constexpr std::uint32_t kTolerancePercent = 15u;
    const std::uint64_t difference = measuredHz > expectedHz
        ? static_cast<std::uint64_t>(measuredHz - expectedHz)
        : static_cast<std::uint64_t>(expectedHz - measuredHz);
    return difference * 100u <= static_cast<std::uint64_t>(expectedHz) * kTolerancePercent;
}

inline std::uint32_t edgeFrequencyHz(const std::uint32_t edges, const std::uint32_t elapsedMs) {
    if (elapsedMs == 0u) {
        return 0u;
    }
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(edges) * 1000u + elapsedMs / 2u) / elapsedMs);
}

}  // namespace stm32_i2s_loopback

#endif // BLASTER_STM32F411_I2S_GPIO_LOOPBACK_H
