#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "audio/IAudioBackend.h"

class IDebug;
class AudioFileSourceSD;
class AudioGeneratorWAV;
class AudioOutputI2S;
class AudioOutputMixer;
class AudioOutputMixerStub;

class OverlapESPAudioBackend : public IAudioBackend {
public:
    static constexpr std::size_t kAbsoluteMaxVoices = 8;

    explicit OverlapESPAudioBackend(std::size_t maxVoices,
                                    IDebug* debug = nullptr);
    ~OverlapESPAudioBackend() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file) override;
    void stop() override;

private:
    struct Voice {
        AudioFileSourceSD* file = nullptr;
        AudioGeneratorWAV* wav = nullptr;
        AudioOutputMixerStub* stub = nullptr;
        bool active = false;
    };

    Voice* findFreeVoice();
    Voice* stealOldestVoice();
    void stopVoice(Voice& voice);

private:
    IDebug* m_debug = nullptr;
    AudioOutputI2S* m_out = nullptr;
    AudioOutputMixer* m_mixer = nullptr;

    std::array<Voice, kAbsoluteMaxVoices> m_voices{};
    std::array<uint32_t, kAbsoluteMaxVoices> m_voiceAge{};
    std::size_t m_voiceCount = 0;
    uint32_t m_nextAge = 1;
};