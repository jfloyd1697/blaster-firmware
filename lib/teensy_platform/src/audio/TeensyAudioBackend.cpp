#include "platform/teensy/audio/TeensyAudioBackend.h"

#include <Arduino.h>
#include <SD.h>

#include "core/debug/IDebug.h"

namespace {
constexpr int kAudioMemoryBlocks = 16;
}

TeensyAudioBackend::TeensyAudioBackend(IDebug* debug, const float volume)
    : m_debug(debug),
      m_volume(volume),
      m_leftConnection(m_player, 0, m_output, 0),
      m_rightConnection(m_player, 1, m_output, 1) {
}

bool TeensyAudioBackend::begin() {
    AudioMemory(kAudioMemoryBlocks);

    if (!m_codec.enable()) {
        if (m_debug != nullptr) {
            m_debug->error("TeensyAudioBackend: SGTL5000 enable failed");
        }
        return false;
    }

    m_codec.volume(m_volume);

    if (m_debug != nullptr) {
        m_debug->log("TeensyAudioBackend: Audio Shield initialized");
    }

    return true;
}

void TeensyAudioBackend::update() {
    if (!m_active || m_player.isPlaying()) {
        return;
    }

    if (m_loop) {
        if (!startCurrentFile()) {
            stop();
        }
        return;
    }

    m_active = false;
    m_currentFile.clear();

    if (m_debug != nullptr) {
        m_debug->log("TeensyAudioBackend: playback finished");
    }
}

void TeensyAudioBackend::playSound(const std::string& file, const bool loop, const bool blocking) {
    stop();

    m_currentFile = normalizePath(file);
    m_loop = loop;

    if (!SD.exists(m_currentFile.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("TeensyAudioBackend: file does not exist: " + m_currentFile);
        }
        m_currentFile.clear();
        m_loop = false;
        return;
    }

    if (!startCurrentFile()) {
        stop();
        return;
    }

    if (blocking && !loop) {
        while (m_player.isPlaying()) {
            yield();
        }
        m_active = false;
        m_currentFile.clear();
    }
}

void TeensyAudioBackend::stop() {
    if (m_player.isPlaying()) {
        m_player.stop();
    }

    m_currentFile.clear();
    m_loop = false;
    m_active = false;
}

std::string TeensyAudioBackend::normalizePath(const std::string& path) {
    std::string normalized = path;
    for (char& ch : normalized) {
        if (ch == '\\') {
            ch = '/';
        }
    }
    return normalized;
}

bool TeensyAudioBackend::startCurrentFile() {
    if (m_currentFile.empty()) {
        return false;
    }

    if (!m_player.play(m_currentFile.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("TeensyAudioBackend: failed to play " + m_currentFile);
        }
        return false;
    }

    m_active = true;

    if (m_debug != nullptr) {
        m_debug->log("TeensyAudioBackend: playing " + m_currentFile);
    }

    return true;
}
