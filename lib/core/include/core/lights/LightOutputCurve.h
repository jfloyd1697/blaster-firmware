#ifndef BLASTER_CORE_LIGHT_OUTPUT_CURVE_H
#define BLASTER_CORE_LIGHT_OUTPUT_CURVE_H

#include <array>
#include <cstdint>

#include "core/lights/ILightBackend.h"

// WS2812 calibration: perceptual gamma ~2, a conservative global output limit,
// and less green drive to compensate for green's greater apparent luminance.
// These constants intentionally affect the physical output only; frame-authored
// RGB values and binary sequence files stay portable and unchanged.
struct LightOutputCurve {
    static constexpr int RedPeak = 94;
    static constexpr int GreenPeak = 59;
    static constexpr int BluePeak = 88;
    static constexpr int MaxInput = 255;

    static constexpr std::uint8_t channel(const std::uint8_t input, const int peak) {
        return static_cast<std::uint8_t>(
            (static_cast<int>(input) * static_cast<int>(input) * peak + 32512)
                / (MaxInput * MaxInput));
    }

    static LightColor correct(const LightColor& input) {
        return {channel(input[0], RedPeak),
                channel(input[1], GreenPeak),
                channel(input[2], BluePeak)};
    }
};

#endif
