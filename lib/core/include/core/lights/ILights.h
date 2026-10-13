#ifndef BLASTER_FIRMWARE_ILIGHTS_H
#define BLASTER_FIRMWARE_ILIGHTS_H

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

class ITextResourceLoader;

namespace weapon_behavior {
struct LightPatternDef;
}

struct LightPixel {
    std::size_t index = 0;
    std::array<int, 3> color{0, 0, 0};
    float intensity = 1.0F;
};

struct LightFrame {
    std::uint32_t durationMs = 0;
    std::vector<LightPixel> pixels;
};

struct LightSequence {
    std::size_t ledCount = 0;
    std::vector<LightFrame> frames;
};

class ILights {
public:
    virtual ~ILights() = default;

    virtual void setPattern(const weapon_behavior::LightPatternDef& pattern) = 0;
    virtual void flashPattern(const weapon_behavior::LightPatternDef& pattern) = 0;
    virtual void flash() = 0;
    virtual void playSequence(const LightSequence& sequence, bool loop) = 0;
    virtual bool playSequenceFile(
        const std::string& path, ITextResourceLoader& loader, bool loop) {
        (void)path;
        (void)loader;
        (void)loop;
        return false;
    }
    // Queue a sequence for storage servicing. Implementations without deferred
    // loading retain the synchronous behavior.
    virtual bool requestSequenceFile(
        const std::string& path, ITextResourceLoader& loader, bool loop) {
        return playSequenceFile(path, loader, loop);
    }
    virtual void stopSequence() = 0;
    virtual void update() = 0;
};

#endif // BLASTER_FIRMWARE_ILIGHTS_H
