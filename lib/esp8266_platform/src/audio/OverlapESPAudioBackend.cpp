#include <Arduino.h>
#include <SD.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"

#include "core/debug/IDebug.h"
#include "platform/esp8266/audio/OverlapESPAudioBackend.h"


OverlapESPAudioBackend::OverlapESPAudioBackend(const std::size_t maxVoices,
                                               IDebug* debug)
    : m_debug(debug) {
    if (maxVoices == 0) {
        m_voiceCount = 1;
    } else if (maxVoices > kAbsoluteMaxVoices) {
        m_voiceCount = kAbsoluteMaxVoices;
    } else {
        m_voiceCount = maxVoices;
    }

    m_voiceAge.fill(0);
}

OverlapESPAudioBackend::~OverlapESPAudioBackend() {
    stop();

    if (m_mixer != nullptr) {
        delete m_mixer;
        m_mixer = nullptr;
    }

    if (m_out != nullptr) {
        delete m_out;
        m_out = nullptr;
    }
}

bool OverlapESPAudioBackend::begin() {
    m_out = new AudioOutputI2S();
    if (m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: failed to allocate I2S output");
        }
        return false;
    }

    m_out->SetPinout(3, D8, D4);
    m_out->SetGain(1.0f);

    m_mixer = new AudioOutputMixer(128, m_out);
    if (m_mixer == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: failed to allocate mixer");
        }
        return false;
    }

    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        m_voices[i].stub = m_mixer->NewInput();
        if (m_voices[i].stub == nullptr) {
            if (m_debug != nullptr) {
                m_debug->error("OverlapESPAudioBackend: failed to allocate mixer input");
            }
            return false;
        }
        m_voices[i].stub->SetGain(0.75f);
    }

    if (m_debug != nullptr) {
        m_debug->log("OverlapESPAudioBackend: mixer initialized");
    }

    return true;
}

void OverlapESPAudioBackend::update() {
    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        Voice& voice = m_voices[i];

        if (voice.active && voice.wav != nullptr) {
            if (!voice.wav->isRunning() || !voice.wav->loop()) {
                stopVoice(voice);
                m_voiceAge[i] = 0;
            }
        }
    }

    if (m_mixer != nullptr) {
        m_mixer->loop();
    }
}

void OverlapESPAudioBackend::playSound(const std::string& file) {
    if (m_mixer == nullptr || m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: output not initialized");
        }
        return;
    }

    if (!SD.exists(file.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: file does not exist: " + file);
        }
        return;
    }

    Voice* voice = findFreeVoice();
    if (voice == nullptr) {
        voice = stealOldestVoice();
    }

    if (voice == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: no voice available");
        }
        return;
    }

    stopVoice(*voice);

    voice->file = new AudioFileSourceSD(file.c_str());
    if (voice->file == nullptr || !voice->file->isOpen()) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: failed to open " + file);
        }
        stopVoice(*voice);
        return;
    }

    voice->wav = new AudioGeneratorWAV();
    if (voice->wav == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: failed to allocate WAV generator");
        }
        stopVoice(*voice);
        return;
    }

    if (!voice->wav->begin(voice->file, voice->stub)) {
        if (m_debug != nullptr) {
            m_debug->error("OverlapESPAudioBackend: failed to start WAV " + file);
        }
        stopVoice(*voice);
        return;
    }

    voice->active = true;

    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        if (&m_voices[i] == voice) {
            m_voiceAge[i] = m_nextAge++;
            break;
        }
    }

    if (m_debug != nullptr) {
        m_debug->log("OverlapESPAudioBackend: playing " + file);
    }
}

void OverlapESPAudioBackend::stop() {
    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        stopVoice(m_voices[i]);
        m_voiceAge[i] = 0;
    }
}

OverlapESPAudioBackend::Voice* OverlapESPAudioBackend::findFreeVoice() {
    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        if (!m_voices[i].active) {
            return &m_voices[i];
        }
    }

    return nullptr;
}

OverlapESPAudioBackend::Voice* OverlapESPAudioBackend::stealOldestVoice() {
    std::size_t oldestIndex = kAbsoluteMaxVoices;
    uint32_t oldestAge = 0xFFFFFFFFu;

    for (std::size_t i = 0; i < m_voiceCount; ++i) {
        if (m_voices[i].active && m_voiceAge[i] < oldestAge) {
            oldestAge = m_voiceAge[i];
            oldestIndex = i;
        }
    }

    if (oldestIndex == kAbsoluteMaxVoices) {
        return nullptr;
    }

    return &m_voices[oldestIndex];
}

void OverlapESPAudioBackend::stopVoice(Voice& voice) {
    if (voice.wav != nullptr) {
        voice.wav->stop();
        delete voice.wav;
        voice.wav = nullptr;
    }

    if (voice.file != nullptr) {
        delete voice.file;
        voice.file = nullptr;
    }

    voice.active = false;
}