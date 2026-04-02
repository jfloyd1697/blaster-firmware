//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_PCPLATFORM_H
#define BLASTER_PCPLATFORM_H

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>

#include "audio/AudioEngine.h"
#include "core/Platform.h"
#include "core/debug/IDebug.h"
#include "core/time/ITime.h"
#include "core/weapons/IWeaponLoader.h"

#include "platform/pc/input/PCInput.h"
#include "platform/pc/audio/PCAudioBackend.h"
#include "platform/pc/PCPlatform.h"


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

// ------------------------- PC Weapon Loader -------------------------


class PCWeaponLoader : public IWeaponLoader {
protected:
    std::string loadText(const std::string& path, IDebug* debug) override {
        namespace fs = std::filesystem;

        if (!fs::exists(path)) {
            debug->error("PCWeaponLoader: file not found: " + path);

            // Dump current working directory
            const auto cwd = fs::current_path();
            debug->error("Current working directory: " + cwd.string());

            debug->error("Directory dump:");

            dumpDirectory(cwd, debug, 0, 2); // limit depth to avoid spam

            return {};
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            debug->error("PCWeaponLoader: failed to open file: " + path);
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

private:
    void dumpDirectory(const std::filesystem::path& dir,
                       IDebug* debug,
                       int depth,
                       int maxDepth) {
        namespace fs = std::filesystem;

        if (depth > maxDepth) return;

        for (const auto& entry : fs::directory_iterator(dir)) {
            std::string indent(depth * 2, ' ');
            std::string name = entry.path().filename().string();

            if (entry.is_directory()) {
                debug->log(indent + "[D] " + name);
                dumpDirectory(entry.path(), debug, depth + 1, maxDepth);
            } else {
                debug->log(indent + "[F] " + name);
            }
        }
    }
};


// ------------------------- PC Platform Factory -------------------------
PlatformServices PCPlatformFactory::create() {
    PlatformServices services;
    services.debug = std::make_unique<PCDebug>();
    services.time = std::make_unique<PCTime>();
    services.input = std::make_unique<PCInput>(services.time.get());

    auto audio = std::make_unique<AudioEngine>(
        std::make_unique<PCAudioBackend>(services.debug.get())
    );

    if (!audio->begin() && services.debug) {
        services.debug->error("PCPlatform: audio begin failed");
    }

    services.audio = std::move(audio);

    services.loader = std::make_unique<PCWeaponLoader>();
    services.assetRoot = "assets/"; // Default asset root
    return services;
}

#endif //BLASTER_PCPLATFORM_H
