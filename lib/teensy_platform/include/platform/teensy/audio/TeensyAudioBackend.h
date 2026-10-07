#ifndef BLASTER_TEENSY_AUDIO_BACKEND_H
#define BLASTER_TEENSY_AUDIO_BACKEND_H

#pragma once

#include <Audio.h>

#include <string>

#include "audio/IAudioBackend.h"

struct IDebug;

class TeensyAudioBackend : public IAudioBackend {
public:
    explicit TeensyAudioBackend(IDebug* debug = nullptr, float volume = 0.65F);

    bool begin() override;
    void update() override;
    void playSound(const std::string& file, bool loop, bool blocking) override;
    void stop() override;

private:
    [[nodiscard]] static std::string normalizePath(const std::string& path);
    bool startCurrentFile();

    IDebug* m_debug = nullptr;
    float m_volume = 0.65F;
    std::string m_currentFile;
    bool m_loop = false;
    bool m_active = false;

    AudioPlaySdWav m_player;
    AudioOutputI2S m_output;
    AudioConnection m_leftConnection;
    AudioConnection m_rightConnection;
    AudioControlSGTL5000 m_codec;
};

#endif // BLASTER_TEENSY_AUDIO_BACKEND_H
