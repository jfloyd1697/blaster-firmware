//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_SOUNDBANK_H
#define BLASTER_FIRMWARE_SOUNDBANK_H

#include <vector>
#include "core/weapons/WeaponProfile.h"

struct SoundBank {
    std::string name;
    std::vector<WeaponProfile> weapons;
};


#endif //BLASTER_FIRMWARE_SOUNDBANK_H