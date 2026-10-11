#ifndef BLASTER_FIRMWARE_CORE_IAUDIOBACKEND_H
#define BLASTER_FIRMWARE_CORE_IAUDIOBACKEND_H

#pragma once

#include <cstdint>
#include <string>

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual bool begin() = 0;

    virtual void update() = 0;

    virtual void playSound(const std::string &file, bool loop, bool blocking) = 0;

    // Hardware diagnostic tone; returns false on backends without tone synthesis.
    // Does not use a file, SD card, or physical input.
    virtual bool playTone(std::uint32_t frequencyHz, std::uint32_t durationMs) {
        (void) frequencyHz;
        (void) durationMs;
        return false;
    }

    virtual void stop() = 0;
};

#endif // BLASTER_FIRMWARE_CORE_IAUDIOBACKEND_H
