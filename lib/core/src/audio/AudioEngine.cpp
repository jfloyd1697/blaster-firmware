#include <utility>

#include "core/audio/AudioEngine.h"
#include "core/audio/IAudioBackend.h"

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

void AudioEngine::playSound(const std::string &file, const bool loop, const bool blocking) {
    if (m_backend != nullptr) {
        m_backend->playSound(file, loop, blocking);
    }
}

bool AudioEngine::playTone(const std::uint32_t frequencyHz, const std::uint32_t durationMs) {
    return m_backend != nullptr && m_backend->playTone(frequencyHz, durationMs);
}

void AudioEngine::stop() {
    if (m_backend != nullptr) {
        m_backend->stop();
    }
}
