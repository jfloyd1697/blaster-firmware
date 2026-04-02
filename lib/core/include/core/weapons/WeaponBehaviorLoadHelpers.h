//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORLOADHELPERS_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORLOADHELPERS_H


#pragma once

#include <string>

#include "weapon_behavior/WeaponBehaviorTypes.h"


class ITextResourceLoader;


namespace weapon_behavior {

    WeaponBehaviorDef loadWeaponBehavior(
        ITextResourceLoader& loader,
        const std::string& path);

    struct LoadWeaponBehaviorResult {
        bool ok = false;
        std::optional<WeaponBehaviorDef> behavior;
        std::vector<std::string> errors;
    };

    LoadWeaponBehaviorResult tryLoadWeaponBehavior(
        ITextResourceLoader& loader,
        const std::string& path);

} // namespace weapon_behavior

#endif //BLASTER_FIRMWARE_WEAPONBEHAVIORLOADHELPERS_H