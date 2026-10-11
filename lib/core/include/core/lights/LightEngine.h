#ifndef BLASTER_FIRMWARE_LIGHTENGINE_H
#define BLASTER_FIRMWARE_LIGHTENGINE_H

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "core/lights/ILightBackend.h"
#include "core/lights/ILights.h"
#include "core/lights/LightSequenceParser.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class ITime;

class LightEngine : public ILights {
public:
    LightEngine(std::unique_ptr<ILightBackend> backend, ITime* time);

    bool begin();

    void setPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flashPattern(const weapon_behavior::LightPatternDef& pattern) override;
    void flash() override;
    void playSequence(const LightSequence& sequence, bool loop) override;
    bool playSequenceFile(const std::string& path, ITextResourceLoader& loader, bool loop) override;
    void stopSequence() override;
    void update() override;

private:
    struct ActivePattern {
        weapon_behavior::LightPatternDef pattern;
        std::uint32_t startMs = 0;
    };

    struct ActiveSequence {
        LightSequence sequence;
        std::uint32_t startMs = 0;
        bool loop = false;
    };

    struct StreamingSequence {
        ITextResourceLoader* loader = nullptr;
        std::string path;
        LightSequenceHeader header;
        LightFrame frame;
        std::uint32_t startMs = 0;
        std::uint32_t frameEndMs = 0;
        std::uint32_t frameIndex = 0;
        std::size_t nextOffset = LightSequenceParser::FirstFrameOffset;
        bool loop = false;
    };

    struct PatternState {
        std::array<int, 3> color{0, 0, 0};
        int brightness = 0;
        bool finished = false;
    };

    [[nodiscard]] std::uint32_t nowMs() const;
    [[nodiscard]] static PatternState evaluatePattern(
        const weapon_behavior::LightPatternDef& pattern,
        std::uint32_t elapsedMs,
        bool loop);
    [[nodiscard]] static LightColor scaledColor(const std::array<int, 3>& color, int brightness);
    [[nodiscard]] static LightColor scaledColor(const LightPixel& pixel);

    void renderPatternState(const PatternState& state);
    void renderSequenceFrame(const LightFrame& frame);
    void renderPixels(std::vector<LightColor> pixels);
    void clear();

    std::unique_ptr<ILightBackend> m_backend;
    ITime* m_time = nullptr;
    std::optional<ActivePattern> m_basePattern;
    std::optional<ActivePattern> m_overridePattern;
    std::optional<ActiveSequence> m_sequence;
    std::optional<StreamingSequence> m_stream;
    std::vector<LightColor> m_lastPixels;
};

#endif // BLASTER_FIRMWARE_LIGHTENGINE_H
