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
#include "core/weapons/SoundBank.h"
#include "weapons/IWeaponLoader.h"


struct PlatformServices {
    std::unique_ptr<IAudioEngine> audio = nullptr;
    std::unique_ptr<IInput> input = nullptr;
    std::unique_ptr<ITime> time = nullptr;
    std::unique_ptr<IDebug> debug = nullptr;
    std::unique_ptr<ILights> lights = nullptr;
    std::unique_ptr<IWeaponLoader> loader = nullptr;
    std::string assetRoot = "assets/";

    PlatformServices() = default;

    ~PlatformServices() = default;

    [[nodiscard]] auto loadSoundBanks(const std::string &path) const {
        return loader->loadSoundBanks(path);
    }


    PlatformServices(const PlatformServices &) = delete; // no copy
    PlatformServices &operator=(const PlatformServices &) = delete; // no copy

    PlatformServices(PlatformServices &&) noexcept = default; // allow move
    PlatformServices &operator=(PlatformServices &&) noexcept = default;
};

#endif //BLASTER_FIRMWARE_PLATFORM_H
