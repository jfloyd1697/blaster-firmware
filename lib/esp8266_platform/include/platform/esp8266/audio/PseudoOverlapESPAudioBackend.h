#ifndef BLASTER_FIRMWARE_PSEUDOOVERLAPESPAUDIOBACKEND_H
#define BLASTER_FIRMWARE_PSEUDOOVERLAPESPAUDIOBACKEND_H


#pragma once

#include <string>

#include "audio/IAudioBackend.h"
#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

class AudioFileSourceSD;
class AudioGeneratorWAV;
class AudioOutputI2S;
class AudioOutputMixer;
class AudioOutputMixerStub;

class PseudoOverlapESPAudioBackend : public IAudioBackend {
public:
    explicit PseudoOverlapESPAudioBackend(ITime* time,
                                          IDebug* debug = nullptr,
                                          uint32_t overlapMs = 30);
    ~PseudoOverlapESPAudioBackend() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file) override;
    void stop() override;

protected:
    struct Voice {
        AudioFileSourceSD* file = nullptr;
        AudioGeneratorWAV* wav = nullptr;
        AudioOutputMixerStub* stub = nullptr;
        bool active = false;
        uint32_t forcedStopAtMs = 0;
    };

    void stopVoice(Voice& voice);
    Voice* getFreeVoice();
    Voice* getOldestActiveVoice();
    Voice* getNewestActiveVoice();
    void startVoice(Voice& voice, const std::string& file);
    uint32_t nowMs() const;

private:
    static constexpr uint32_t kMixerBufferSamples = 128;
    static constexpr int kVoiceCount = 2;

    ITime* m_time = nullptr;
    IDebug* m_debug = nullptr;
    uint32_t m_overlapMs = 30;

    AudioOutputI2S* m_out = nullptr;
    AudioOutputMixer* m_mixer = nullptr;

    Voice m_voices[kVoiceCount]{};
    uint32_t m_voiceAge[kVoiceCount]{};
    uint32_t m_nextAge = 1;
};


#endif //BLASTER_FIRMWARE_PSEUDOOVERLAPESPAUDIOBACKEND_H