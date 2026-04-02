#ifndef BLASTER_FIRMWARE_WEAPON_PROFILE_H
#define BLASTER_FIRMWARE_WEAPON_PROFILE_H

#pragma once

#include <cstdint>
#include <string>

#include "core/weapons/ShootModeType.h"

struct WeaponSounds {
    std::string idle;
    std::string shot;
    std::string chargeStart;
    std::string chargeLoop;
    std::string chargeRelease;
    std::string reload;
    std::string empty;
    std::string equip;
};

struct ShootModeConfig {
    ShootModeType type = ShootModeType::Single;

    uint32_t cooldownMs = 100;

    int burstCount = 3;
    uint32_t burstIntervalMs = 60;

    uint32_t minChargeMs = 300;
    uint32_t maxChargeMs = 1200;
};

struct WeaponProfile {
    std::string name;
    WeaponSounds sounds;
    ShootModeConfig shootMode;

    int ammoPerShot = 0;
    int magazineSize = 0;
};

#endif