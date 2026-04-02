#ifndef BLASTER_FIRMWARE_AUDIOENGINE_H
#define BLASTER_FIRMWARE_AUDIOENGINE_H

#pragma once

#include <memory>
#include <string>

#include "core/audio/IAudioEngine.h"

class IAudioBackend;

class AudioEngine : public IAudioEngine {
public:
    explicit AudioEngine(std::unique_ptr<IAudioBackend> backend);
    ~AudioEngine() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file) override;
    void stop() override;

private:
    std::unique_ptr<IAudioBackend> m_backend;
};
#endif //BLASTER_FIRMWARE_AUDIOENGINE_H
