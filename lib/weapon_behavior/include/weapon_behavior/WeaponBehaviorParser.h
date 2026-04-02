//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H

#pragma once

#include <string>
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace weapon_behavior {
    class WeaponBehaviorParser {
    public:
        static WeaponBehaviorDef parseFromText(const std::string &jsonText);
    };
} // namespace weapon_behavior

#endif //BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
