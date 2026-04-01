//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_PLATFORM_H
#define BLASTER_FIRMWARE_PLATFORM_H

#pragma once
#include <memory>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/lights/ILights.h"
#include "core/time/ITime.h"
#include "core/input/IInput.h"
#include "core/weapons/IWeaponLoader.h"


struct PlatformServices {
    std::unique_ptr<IAudioEngine> audio;
    std::unique_ptr<IInput> input;
    std::unique_ptr<ITime> time;
    std::unique_ptr<IDebug> debug;
    std::unique_ptr<Lights::ILights> lights;
    std::unique_ptr<IWeaponLoader> loader;
    std::string assetRoot;

    PlatformServices() = default;

    ~PlatformServices() = default;

    PlatformServices(const PlatformServices &) = delete; // no copy
    PlatformServices &operator=(const PlatformServices &) = delete; // no copy

    PlatformServices(PlatformServices &&) noexcept = default; // allow move
    PlatformServices &operator=(PlatformServices &&) noexcept = default;
};

#endif //BLASTER_FIRMWARE_PLATFORM_H
