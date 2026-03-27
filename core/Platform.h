//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_PLATFORM_H
#define BLASTER_FIRMWARE_PLATFORM_H

#pragma once
#include <memory>
#include "IAudioEngine.h"
#include "IInput.h"
#include "ITime.h"
#include "IDebug.h"

struct PlatformServices {
    std::unique_ptr<IAudioEngine> audio;
    std::unique_ptr<IInput> input;
    std::unique_ptr<ITime> time;
    std::unique_ptr<IDebug> debug;
    std::string assetRoot;

    PlatformServices() = default;
    ~PlatformServices() = default;

    PlatformServices(const PlatformServices&) = delete;             // no copy
    PlatformServices& operator=(const PlatformServices&) = delete;  // no copy

    PlatformServices(PlatformServices&&) noexcept = default;        // allow move
    PlatformServices& operator=(PlatformServices&&) noexcept = default;
};

#endif //BLASTER_FIRMWARE_PLATFORM_H