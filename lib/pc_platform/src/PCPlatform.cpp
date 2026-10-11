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

#include "core/audio/AudioEngine.h"
#include "core/Platform.h"
#include "core/debug/IDebug.h"
#include "core/storage/ResourceTreeDumper.h"
#include "core/time/ITime.h"

#include "platform/pc/input/PCInput.h"
#include "platform/pc/audio/PCAudioBackend.h"
#include "platform/pc/PCPlatform.h"
#include "platform/pc/text_resource_loader/PCTextResourceLoader.h"


// ------------------------- PC Debug -------------------------
struct PCDebug : IDebug {
    void log(const std::string &msg) override {
        std::cout << "[LOG] " << msg << std::endl;
    }

    void error(const std::string &msg) override {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
};

// ------------------------- PC Time -------------------------

struct PCTime : ITime {
    unsigned long millis() const override {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                .count();
    }
};




// ------------------------- PC Platform Factory -------------------------
PlatformServices PCPlatformFactory::create() {
    PlatformServices services;
    services.debug = std::make_unique<PCDebug>();
    services.debug->log("PCPlatform: service startup begin");

    services.debug->log("PCPlatform: starting time service");
    services.time = std::make_unique<PCTime>();
    services.debug->log("PCPlatform: time service ready");

    services.debug->log("PCPlatform: starting input service");
    services.input = std::make_unique<PCInput>(services.time.get());
    services.debug->log("PCPlatform: input service ready");

    services.debug->log("PCPlatform: starting audio service");
    auto audio = std::make_unique<AudioEngine>(
        std::make_unique<PCAudioBackend>(services.debug.get())
    );

    if (!audio->begin()) {
        services.debug->error("PCPlatform: audio service failed");
    } else {
        services.debug->log("PCPlatform: audio service ready");
    }

    services.audio = std::move(audio);

    services.debug->log("PCPlatform: starting storage service");
    services.textLoader = std::make_unique<PCTextResourceLoader>(services.debug.get());
    services.debug->log("PCPlatform: storage service ready");
    ResourceTreeDumper::dump(*services.textLoader, *services.debug);

    services.debug->log("PCPlatform: service startup complete");
    return services;
}

#endif //BLASTER_PCPLATFORM_H
