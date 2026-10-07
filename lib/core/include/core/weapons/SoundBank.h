#ifndef BLASTER_FIRMWARE_SOUNDBANK_H
#define BLASTER_FIRMWARE_SOUNDBANK_H

#pragma once

#include <string>
#include <vector>

#include "weapon_behavior/WeaponBehaviorTypes.h"

struct SoundBank {
    std::string name;
    std::vector<weapon_behavior::WeaponBehaviorDef> weapons;
};


struct WeaponEntry {
    std::string name;
    std::string behaviorPath;
    ;
};


struct WeaponBank {
    std::string name;
    std::vector<WeaponEntry> weapons;
};

#endif // BLASTER_FIRMWARE_SOUNDBANK_H
