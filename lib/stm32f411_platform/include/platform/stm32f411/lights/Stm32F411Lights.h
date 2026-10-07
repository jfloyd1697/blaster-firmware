#ifndef BLASTER_STM32F411_LIGHTS_H
#define BLASTER_STM32F411_LIGHTS_H

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "core/lights/ILights.h"
#include "platform/stm32f411/board/Stm32F411BoardConfig.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class Stm32F411Lights : public ILights {
public:
    explicit Stm32F411Lights(const Stm32F411LightPins& pins);
    ~Stm32F411Lights() override;

    bool begin();

    void setPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flashPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flash() override;
    void update() override;
    void onDmaComplete();

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

    bool initializeTimerDma();
    void render(const RenderState& state);
    void transmitColor(std::uint32_t color);
    void waitForTransfer();

    Stm32F411LightPins m_pins;
    std::vector<std::uint16_t> m_pwmData;
    std::optional<ActivePattern> m_basePattern;
    std::optional<ActivePattern> m_overridePattern;
    std::uint32_t m_lastColor = 0xFFFFFFFFu;
    bool m_transferActive = false;
};

#endif // BLASTER_STM32F411_LIGHTS_H
