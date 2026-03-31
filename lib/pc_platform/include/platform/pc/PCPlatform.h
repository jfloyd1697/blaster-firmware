//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_PCPLATFORM_H
#define BLASTER_PCPLATFORM_H

#pragma once

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <windows.h>    // For WinMM audio
#include <fstream>
#include <sstream>

#include "PCInput.h"

// ------------------------- PC Debug -------------------------
struct PCDebug : public IDebug {
    void log(const std::string &msg) override {
        std::cout << "[LOG] " << msg << std::endl;
    }

    void error(const std::string &msg) override {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
};

// ------------------------- PC Time -------------------------

struct PCTime : public ITime {
    uint64_t millis() const override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                .count();
    }
};

// ------------------------- PC Audio -------------------------

struct PCAudioEngine : public IAudioEngine {
    explicit PCAudioEngine(IDebug *debugService = nullptr)
        : m_debug(debugService) {
    }

    void update() override {
    }

    void playSound(const std::string &file) override {
        if (m_debug != nullptr) {
            if (!std::filesystem::exists(file)) {
                m_debug->error("PCAudio: Sound file NOT FOUND: " + file);
            } else {
                m_debug->log("PCAudio: Sound file found: " + file);
            }
        }

        PlaySoundA(file.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void stop() override {
        PlaySoundA(nullptr, nullptr, 0);
    }

private:
    IDebug *m_debug;
};


class PCWeaponLoader : public IWeaponLoader {
protected:
    std::string loadText(const std::string& path) override {
        std::ifstream file(path);
        if (!file.is_open()) {
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};


// ------------------------- PC Platform Factory -------------------------
struct PCPlatformFactory {
    static PlatformServices create() {
        PlatformServices services;
        services.debug = std::make_unique<PCDebug>();
        services.input = std::make_unique<PCInput>();
        services.time = std::make_unique<PCTime>();
        services.audio = std::make_unique<PCAudioEngine>(services.debug.get());
        services.assetRoot = "assets/"; // Default asset root
        return services;
    }
};
#endif //BLASTER_PCPLATFORM_H
