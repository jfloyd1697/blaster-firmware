#ifndef BLASTER_ESPAUDIOENGINE_H
#define BLASTER_ESPAUDIOENGINE_H

#pragma once

#include <string>

#include "IAudioEngine.h"
#include "IDebug.h"

class AudioGeneratorWAV;
class AudioFileSourceSD;
class AudioOutputI2S;

class ESPAudioEngine final : public IAudioEngine {
public:
    ESPAudioEngine() = default;

    explicit ESPAudioEngine(IDebug *debug = nullptr) : m_debug(debug) {
    };

    ~ESPAudioEngine() override;

    bool begin(int sdCsPin);

    void update() override;

    void playSound(const std::string &file) override;

    void stop() override;

private:
    IDebug *m_debug = nullptr;
    AudioGeneratorWAV *m_wav = nullptr;
    AudioFileSourceSD *m_file = nullptr;
    AudioOutputI2S *m_out = nullptr;
};

#endif
