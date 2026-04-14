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
#include "text_resource_loader/ITextResourceLoader.h"
#include "weapons/IWeaponLoader.h"
#include "weapons/SoundBank.h"
#include "weapons/WeaponBehaviorLoadHelpers.h"


struct PlatformServices {
    std::unique_ptr<IAudioEngine> audio = nullptr;
    std::unique_ptr<IInput> input = nullptr;
    std::unique_ptr<ITime> time = nullptr;
    std::unique_ptr<IDebug> debug = nullptr;
    std::unique_ptr<ILights> lights = nullptr;
    std::unique_ptr<ITextResourceLoader> text_loader = nullptr;
    std::unique_ptr<IWeaponLoader> weapon_loader = nullptr;
    std::string assetRoot = "assets/";

    PlatformServices() = default;

    ~PlatformServices() = default;

    [[nodiscard]] auto loadWeaponBehavior(const std::string &path) const {
        weapon_behavior::WeaponBehaviorDef weapon = weapon_behavior::loadWeaponBehavior(*text_loader, path);
        std::vector<WeaponProfile> profiles = {};
        profiles.push_back(
            WeaponProfile({
                .name = weapon.weapon,
                .behaviorPath = path
            })
        );
    }

    [[nodiscard]] auto loadSoundBanks(const std::string &path) const {
        std::vector<SoundBank> banks = {};
        if (weapon_loader) {
            try {
                banks = weapon_loader->loadSoundBanks(path);
            } catch (const std::exception &e) {
                if (debug) {
                    debug->error("Failed to load sound banks: " + std::string(e.what()));
                }
            }
        } else {
            if (debug) {
                debug->error("Weapon loader not available, cannot load sound banks");
            }
        }
        return banks;
    }


    PlatformServices(const PlatformServices &) = delete; // no copy
    PlatformServices &operator=(const PlatformServices &) = delete; // no copy

    PlatformServices(PlatformServices &&) noexcept = default; // allow move
    PlatformServices &operator=(PlatformServices &&) noexcept = default;
};

#endif //BLASTER_FIRMWARE_PLATFORM_H
