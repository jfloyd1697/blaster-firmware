#include <Arduino.h>
#include <SD.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "core/debug/IDebug.h"
#include "platform/esp8266/audio/BasicESPAudioBackend.h"


BasicESPAudioBackend::BasicESPAudioBackend(IDebug *debug)
    : m_debug(debug) {
}

BasicESPAudioBackend::~BasicESPAudioBackend() {
    stop();

    if (m_out != nullptr) {
        delete m_out;
        m_out = nullptr;
    }
}

bool BasicESPAudioBackend::begin() {
    m_out = new AudioOutputI2S();
    if (m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: failed to allocate I2S output");
        }
        return false;
    }

    m_out->SetPinout(3, D8, D4);
    m_out->SetGain(1.0f);

    if (m_debug != nullptr) {
        m_debug->log("BasicESPAudioBackend: I2S output initialized");
    }

    return true;
}

void BasicESPAudioBackend::update() {
    if (m_wav != nullptr && m_wav->isRunning()) {
        if (!m_wav->loop()) {
            if (m_debug != nullptr) {
                m_debug->log("BasicESPAudioBackend: playback finished");
            }
            stop();
        }
    }
}

void BasicESPAudioBackend::playSound(const std::string &file, const bool loop) {
    stop();

    if (m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: output not initialized");
        }
        return;
    }

    if (!SD.exists(file.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: file does not exist: " + file);
        }
        return;
    }

    m_file = new AudioFileSourceSD(file.c_str());
    if (m_file == nullptr || !m_file->isOpen()) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: failed to open " + file);
        }
        stop();
        return;
    }

    m_wav = new AudioGeneratorWAV();
    if (m_wav == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: failed to allocate WAV generator");
        }
        stop();
        return;
    }

    if (!m_wav->begin(m_file, m_out)) {
        if (m_debug != nullptr) {
            m_debug->error("BasicESPAudioBackend: failed to start WAV " + file);
        }
        stop();
        return;
    }

    if (m_debug != nullptr) {
        m_debug->log("BasicESPAudioBackend: playing " + file);
    }
}

void BasicESPAudioBackend::stop() {
    if (m_wav != nullptr) {
        m_wav->stop();
        delete m_wav;
        m_wav = nullptr;
    }

    if (m_file != nullptr) {
        delete m_file;
        m_file = nullptr;
    }
}
