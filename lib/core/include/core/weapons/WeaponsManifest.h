#ifndef BLASTER_FIRMWARE_WEAPONSMANIFEST_H
#define BLASTER_FIRMWARE_WEAPONSMANIFEST_H

#pragma once

#include <string>
#include <vector>

struct WeaponBankManifest {
    std::string name;
    std::vector<std::string> weapons;
};

struct WeaponsManifest {
    int version = 1;
    std::vector<WeaponBankManifest> banks;
};

#endif // BLASTER_FIRMWARE_WEAPONSMANIFEST_H
