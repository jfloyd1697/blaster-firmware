#include "platform/esp8266/ESPAudioEngine.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"


ESPAudioEngine::~ESPAudioEngine() {
    stop();

    if (m_out != nullptr) {
        delete m_out;
        m_out = nullptr;
    }
}

bool ESPAudioEngine::begin(const int sdCsPin) {
    SPI.begin();

    if (!SD.begin(sdCsPin)) {
        if (m_debug != nullptr) {
            m_debug->error("ESPAudioEngine: SD init failed");
        }
        return false;
    }

    if (m_debug != nullptr) {
        m_debug->log("ESPAudioEngine: SD init OK");
    }

    m_out = new AudioOutputI2S();
    if (m_out == nullptr) {
        if (m_debug != nullptr) {
            m_debug->error("ESPAudioEngine: failed to allocate I2S output");
        }
        return false;
    }

    m_out->SetGain(0.25f);
    return true;
}

void ESPAudioEngine::update() {
    if (m_wav != nullptr && m_wav->isRunning()) {
        if (!m_wav->loop()) {
            stop();
        }
    }
}

void ESPAudioEngine::playSound(const std::string& file) {
    stop();

    m_file = new AudioFileSourceSD(file.c_str());
    if (m_file == nullptr || !m_file->isOpen()) {
        if (m_debug != nullptr) {
            m_debug->error("ESPAudioEngine: failed to open " + file);
        }
        stop();
        return;
    }

    m_wav = new AudioGeneratorWAV();
    if (m_wav == nullptr || !m_wav->begin(m_file, m_out)) {
        if (m_debug != nullptr) {
            m_debug->error("ESPAudioEngine: failed to start WAV " + file);
        }
        stop();
        return;
    }

    if (m_debug != nullptr) {
        m_debug->log("ESPAudioEngine: playing " + file);
    }
}

void ESPAudioEngine::stop() {
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