#include <string>
#include <filesystem>
#include <memory>
#include <windows.h>    // For WinMM audio

#include "core/debug/IDebug.h"
#include "platform/pc/audio/PCAudioBackend.h"

PCAudioBackend::PCAudioBackend(IDebug* debug)
        : m_debug(debug) {
    }

bool PCAudioBackend::begin() {;
    m_debug->log("PCAudioEngine: Initialization complete");
    return true;
}

void PCAudioBackend::update() {
}

void PCAudioBackend::playSound(const std::string &file) {
    if (m_debug != nullptr) {
        if (!std::filesystem::exists(file)) {
            m_debug->error("PCAudio: Sound file NOT FOUND: " + file);
        } else {
            m_debug->log("PCAudio: Sound file found: " + file);
        }
    }

    PlaySoundA(file.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void PCAudioBackend::stop() {
    PlaySoundA(nullptr, nullptr, 0);
}




