#include "../include/audio/ESPAudioEngine.h"
#include "audio/IAudioBackend.h"

ESPAudioEngine::ESPAudioEngine(std::unique_ptr<IAudioBackend> backend)
    : m_backend(std::move(backend)) {
}

ESPAudioEngine::~ESPAudioEngine() = default;

bool ESPAudioEngine::begin() {
    return m_backend != nullptr && m_backend->begin();
}

void ESPAudioEngine::update() {
    if (m_backend != nullptr) {
        m_backend->update();
    }
}

void ESPAudioEngine::playSound(const std::string& file) {
    if (m_backend != nullptr) {
        m_backend->playSound(file);
    }
}

void ESPAudioEngine::stop() {
    if (m_backend != nullptr) {
        m_backend->stop();
    }
}