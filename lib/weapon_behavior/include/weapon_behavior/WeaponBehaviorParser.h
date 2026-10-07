#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H

#pragma once

#include <string>

#include "core/debug/IDebug.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace weapon_behavior {

class WeaponBehaviorParser {
public:
    static WeaponBehaviorDef parseFromText(const std::string& jsonText, IDebug *debug);
};

} // namespace weapon_behavior

#endif // BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
