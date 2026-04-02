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

#include "audio/AudioEngine.h"
#include "core/Platform.h"
#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

#include "platform/pc/input/PCInput.h"
#include "platform/pc/audio/PCAudioBackend.h"
#include "platform/pc/PCPlatform.h"
#include "platform/pc/text_resource_loader/PCTextResourceLoader.h"


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

    services.loader = std::make_unique<PCTextResourceLoader>(services.debug.get());
    services.assetRoot = "assets/"; // Default asset root
    return services;
}

#endif //BLASTER_PCPLATFORM_H
