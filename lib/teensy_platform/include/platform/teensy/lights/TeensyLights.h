#ifndef BLASTER_TEENSY_LIGHTS_H
#define BLASTER_TEENSY_LIGHTS_H

#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "core/lights/ILights.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class OctoWS2811;

class TeensyLights : public ILights {
public:
    TeensyLights();
    ~TeensyLights() override;

    bool begin();

    void setPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flashPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flash() override;
    void update() override;

private:
    struct ActivePattern {
        weapon_behavior::LightPatternDef pattern;
        std::uint32_t startMs = 0;
    };

    struct RenderState {
        std::array<int, 3> color{0, 0, 0};
        int brightness = 0;
        bool finished = false;
    };

    [[nodiscard]] static RenderState renderPattern(
        const weapon_behavior::LightPatternDef& pattern,
        std::uint32_t elapsedMs,
        bool loop);
    [[nodiscard]] static int clampedBrightness(const weapon_behavior::LightPatternDef& pattern);
    void render(const RenderState& state);

    OctoWS2811* m_leds = nullptr;
    std::optional<ActivePattern> m_basePattern;
    std::optional<ActivePattern> m_overridePattern;
    std::uint32_t m_lastColor = 0xFFFFFFFFu;
};

#endif // BLASTER_TEENSY_LIGHTS_H
