#ifndef BLASTER_FIRMWARE_IAUDIOENGINE_H
#define BLASTER_FIRMWARE_IAUDIOENGINE_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual void playSound(const std::string& file, bool loop, bool blocking) = 0;
    virtual bool playTone(std::uint32_t frequencyHz, std::uint32_t durationMs) {
        (void) frequencyHz;
        (void) durationMs;
        return false;
    }

    // Prepare the equipped weapon's unique clips. Hardware backends can keep
    // validated files open so an action never has to open and inspect a WAV.
    // The default is intentionally a no-op for platforms without such caching.
    virtual bool prepareSounds(const std::vector<std::string>& paths) {
        (void) paths;
        return true;
    }
    virtual void releasePreparedSounds() {}

    virtual void stop() = 0;
};

#endif // BLASTER_FIRMWARE_IAUDIOENGINE_H
