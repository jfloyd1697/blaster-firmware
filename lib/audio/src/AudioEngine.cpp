#include <utility>

#include "audio/AudioEngine.h"
#include "audio/IAudioBackend.h"

AudioEngine::AudioEngine(std::unique_ptr<IAudioBackend> backend)
    : m_backend(std::move(backend)) {
}

AudioEngine::~AudioEngine() = default;

bool AudioEngine::begin() {
    return m_backend != nullptr && m_backend->begin();
}

void AudioEngine::update() {
    if (m_backend != nullptr) {
        m_backend->update();
    }
}

void AudioEngine::playSound(const std::string &file, bool loop) {
    if (m_backend != nullptr) {
        m_backend->playSound(file, loop);
    }
}


void AudioEngine::stop() {
    if (m_backend != nullptr) {
        m_backend->stop();
    }
}
