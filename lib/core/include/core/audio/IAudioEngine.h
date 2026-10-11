#ifndef BLASTER_FIRMWARE_IAUDIOENGINE_H
#define BLASTER_FIRMWARE_IAUDIOENGINE_H

#pragma once

#include <cstdint>
#include <string>

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

    virtual void stop() = 0;
};

#endif // BLASTER_FIRMWARE_IAUDIOENGINE_H
