#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H

#pragma once

#include <string>
#include <vector>

#include "core/debug/IDebug.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace weapon_behavior {

struct WeaponAssetReferences {
    bool valid = false;
    std::vector<std::string> audioPaths;
    std::vector<std::string> lightSequencePaths;
};

class WeaponBehaviorParser {
public:
    static WeaponBehaviorDef parse(const std::string& data, IDebug* debug);
    static WeaponBehaviorDef parseFromBinary(const std::string& data, IDebug* debug);
    static WeaponAssetReferences parseAssetReferences(const std::string& data, IDebug* debug);
    static bool isBinaryFormat(const std::string& data);
};

} // namespace weapon_behavior

#endif // BLASTER_FIRMWARE_WEAPONBEHAVIORPARSER_H
