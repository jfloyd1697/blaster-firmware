#pragma once

#include <string>

#include "IAudioBackend.h"

class IDebug;
class AudioFileSourceSD;
class AudioGeneratorWAV;
class AudioOutputI2S;

class BasicESPAudioBackend : public IAudioBackend {
public:
    explicit BasicESPAudioBackend(IDebug* debug = nullptr);
    ~BasicESPAudioBackend() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file) override;
    void stop() override;

private:
    IDebug* m_debug = nullptr;
    AudioFileSourceSD* m_file = nullptr;
    AudioGeneratorWAV* m_wav = nullptr;
    AudioOutputI2S* m_out = nullptr;
};