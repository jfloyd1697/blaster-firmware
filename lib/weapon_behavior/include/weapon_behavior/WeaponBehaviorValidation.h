//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORVALIDATION_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORVALIDATION_H

#pragma once

#include <string>
#include <vector>
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace weapon_behavior {

    struct ValidationResult {
        bool ok = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    ValidationResult validateWeaponBehavior(const WeaponBehaviorDef& def);

} // namespace weapon_behavior

#endif //BLASTER_FIRMWARE_WEAPONBEHAVIORVALIDATION_H