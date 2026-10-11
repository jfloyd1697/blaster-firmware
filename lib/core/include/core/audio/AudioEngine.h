#ifndef BLASTER_FIRMWARE_CORE_AUDIOENGINE_H
#define BLASTER_FIRMWARE_CORE_AUDIOENGINE_H

#pragma once

#include <memory>

#include "core/audio/IAudioEngine.h"

#include "core/audio/IAudioBackend.h"

class AudioEngine : public IAudioEngine {
public:
    explicit AudioEngine(std::unique_ptr<IAudioBackend> backend);

    ~AudioEngine() override;

    bool begin() override;

    void update() override;

    void playSound(const std::string &file, bool loop = false, bool blocking = false) override;

    bool playTone(std::uint32_t frequencyHz, std::uint32_t durationMs) override;

    void stop() override;

private:
    std::unique_ptr<IAudioBackend> m_backend;
};

#endif // BLASTER_FIRMWARE_CORE_AUDIOENGINE_H
