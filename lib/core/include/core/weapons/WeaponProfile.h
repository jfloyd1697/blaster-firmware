#ifndef BLASTER_FIRMWARE_WEAPON_PROFILE_H
#define BLASTER_FIRMWARE_WEAPON_PROFILE_H

#pragma once

#include <string>


struct WeaponProfile {
    std::string name;
    std::string behaviorPath;
};

#endif