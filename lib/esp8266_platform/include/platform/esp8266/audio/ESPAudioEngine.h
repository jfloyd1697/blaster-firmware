#ifndef BLASTER_ESPAUDIOENGINE_H
#define BLASTER_ESPAUDIOENGINE_H

#pragma once

#include <memory>
#include <string>

#include "core/audio/IAudioEngine.h"
#include "audio/IAudioBackend.h"

class ESPAudioEngine : public IAudioEngine {
public:
    explicit ESPAudioEngine(std::unique_ptr<IAudioBackend> backend);
    ~ESPAudioEngine() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file) override;
    void stop() override;

private:
    std::unique_ptr<IAudioBackend> m_backend;
};

#endif
