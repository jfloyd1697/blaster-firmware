#include "PseudoOverlapESPAudioBackend.h"

#include <Arduino.h>
#include <SD.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"
#include "IDebug.h"
#include "ITime.h"

PseudoOverlapESPAudioBackend::PseudoOverlapESPAudioBackend(ITime* time,
                                                           IDebug* debug,
                                                           const uint32_t overlapMs)
    : m_time(time), m_debug(debug), m_overlapMs(overlapMs) {
    if (m_overlapMs == 0) {
        m_overlapMs = 1;
    }

    for (int i = 0; i < kVoiceCount; ++i) {
        m_voiceAge[i] = 0;
    }
}

PseudoOverlapESPAudioBackend::~PseudoOverlapESPAudioBackend() {
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

bool PseudoOverlapESPAudioBackend::begin() {
    m_out = new AudioOutputI2S();
    if (m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: failed to allocate I2S output");
        }
        return false;
    }

    m_out->SetPinout(3, D8, D4);

    // Conservative starting point for ESP8266
    m_out->SetGain(0.5f);

    m_mixer = new AudioOutputMixer(kMixerBufferSamples, m_out);
    if (m_mixer == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: failed to allocate mixer");
        }
        return false;
    }

    for (int i = 0; i < kVoiceCount; ++i) {
        m_voices[i].stub = m_mixer->NewInput();
        if (m_voices[i].stub == nullptr) {
            if (m_debug != nullptr) {
                m_debug->error("PseudoOverlapESPAudioBackend: failed to allocate mixer input");
            }
            return false;
        }

        // Keep low for summed audio
        m_voices[i].stub->SetGain(0.40f);
    }

    if (m_debug != nullptr) {
        m_debug->log("PseudoOverlapESPAudioBackend: initialized");
    }

    return true;
}

void PseudoOverlapESPAudioBackend::update() {
    const uint32_t now = nowMs();

    for (int i = 0; i < kVoiceCount; ++i) {
        Voice& voice = m_voices[i];

        if (!voice.active) {
            continue;
        }

        if (voice.forcedStopAtMs != 0 && now >= voice.forcedStopAtMs) {
            stopVoice(voice);
            m_voiceAge[i] = 0;
            continue;
        }

        if (voice.wav != nullptr) {
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

void PseudoOverlapESPAudioBackend::playSound(const std::string& file) {
    if (m_mixer == nullptr || m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: output not initialized");
        }
        return;
    }

    if (!SD.exists(file.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: file does not exist: " + file);
        }
        return;
    }

    Voice* freeVoice = getFreeVoice();

    if (freeVoice != nullptr) {
        // If another voice is active, let it survive briefly
        Voice* oldVoice = getNewestActiveVoice();
        if (oldVoice != nullptr) {
            oldVoice->forcedStopAtMs = nowMs() + m_overlapMs;
        }

        startVoice(*freeVoice, file);
        return;
    }

    // Both voices are in use. Keep the newest one, kill the older one.
    Voice* oldest = getOldestActiveVoice();
    Voice* newest = getNewestActiveVoice();

    if (oldest == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: no voice available");
        }
        return;
    }

    // Allow the newest voice to hang on briefly for the overlap illusion
    if (newest != nullptr && newest != oldest) {
        newest->forcedStopAtMs = nowMs() + m_overlapMs;
    }

    stopVoice(*oldest);
    startVoice(*oldest, file);
}

void PseudoOverlapESPAudioBackend::stop() {
    for (int i = 0; i < kVoiceCount; ++i) {
        stopVoice(m_voices[i]);
        m_voiceAge[i] = 0;
    }
}

void PseudoOverlapESPAudioBackend::stopVoice(Voice& voice) {
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
    voice.forcedStopAtMs = 0;
}

PseudoOverlapESPAudioBackend::Voice* PseudoOverlapESPAudioBackend::getFreeVoice() {
    for (int i = 0; i < kVoiceCount; ++i) {
        if (!m_voices[i].active) {
            return &m_voices[i];
        }
    }

    return nullptr;
}

PseudoOverlapESPAudioBackend::Voice* PseudoOverlapESPAudioBackend::getOldestActiveVoice() {
    int oldestIndex = -1;
    uint32_t oldestAge = 0xFFFFFFFFu;

    for (int i = 0; i < kVoiceCount; ++i) {
        if (m_voices[i].active && m_voiceAge[i] < oldestAge) {
            oldestAge = m_voiceAge[i];
            oldestIndex = i;
        }
    }

    return oldestIndex >= 0 ? &m_voices[oldestIndex] : nullptr;
}

PseudoOverlapESPAudioBackend::Voice* PseudoOverlapESPAudioBackend::getNewestActiveVoice() {
    int newestIndex = -1;
    uint32_t newestAge = 0;

    for (int i = 0; i < kVoiceCount; ++i) {
        if (m_voices[i].active && m_voiceAge[i] > newestAge) {
            newestAge = m_voiceAge[i];
            newestIndex = i;
        }
    }

    return newestIndex >= 0 ? &m_voices[newestIndex] : nullptr;
}

void PseudoOverlapESPAudioBackend::startVoice(Voice& voice, const std::string& file) {
    stopVoice(voice);

    voice.file = new AudioFileSourceSD(file.c_str());
    if (voice.file == nullptr || !voice.file->isOpen()) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: failed to open " + file);
        }
        stopVoice(voice);
        return;
    }

    voice.wav = new AudioGeneratorWAV();
    if (voice.wav == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: failed to allocate WAV generator");
        }
        stopVoice(voice);
        return;
    }

    if (!voice.wav->begin(voice.file, voice.stub)) {
        if (m_debug != nullptr) {
            m_debug->error("PseudoOverlapESPAudioBackend: failed to start WAV " + file);
        }
        stopVoice(voice);
        return;
    }

    voice.active = true;
    voice.forcedStopAtMs = 0;

    for (int i = 0; i < kVoiceCount; ++i) {
        if (&m_voices[i] == &voice) {
            m_voiceAge[i] = m_nextAge++;
            break;
        }
    }

    if (m_debug != nullptr) {
        m_debug->log("PseudoOverlapESPAudioBackend: playing " + file);
    }
}

uint32_t PseudoOverlapESPAudioBackend::nowMs() const {
    if (m_time != nullptr) {
        return static_cast<uint32_t>(m_time->millis());
    }

    return ::millis();
}