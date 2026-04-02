#include <string>
#include <filesystem>
#include <memory>
#include <windows.h>

#include "core/debug/IDebug.h"
#include "platform/pc/audio/PCAudioBackend.h"

PCAudioBackend::PCAudioBackend(IDebug *debug)
    : m_debug(debug) {
}

bool PCAudioBackend::begin() {
    if (m_debug != nullptr) {
        m_debug->log("PCAudioEngine: Initialization complete");
    }
    return true;
}

void PCAudioBackend::update() {
}

void PCAudioBackend::playSound(const std::string &file, const bool loop) {
    if (m_debug != nullptr) {
        if (!std::filesystem::exists(file)) {
            m_debug->error("PCAudio: Sound file NOT FOUND: " + file);
        } else {
            m_debug->log(
                std::string("PCAudio: Playing ") +
                (loop ? "looping" : "one-shot") +
                " sound: " + file);
        }
    }

    DWORD flags = SND_FILENAME | SND_ASYNC;
    if (loop) {
        flags |= SND_LOOP;
    }

    PlaySoundA(file.c_str(), nullptr, flags);
}

void PCAudioBackend::stop() {
    PlaySoundA(nullptr, nullptr, 0);
}
