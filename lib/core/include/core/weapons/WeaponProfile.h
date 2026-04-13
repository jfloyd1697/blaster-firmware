#ifndef BLASTER_FIRMWARE_WEAPON_PROFILE_H
#define BLASTER_FIRMWARE_WEAPON_PROFILE_H

#pragma once

#include <map>
#include <string>

#include "nlohmann/json.hpp"


struct WeaponProfile {
    std::string name;
    std::string category;
    std::map<std::string, std::string> properties;
    int magazineSize;

    bool static from_json(nlohmann::basic_json<> item, const WeaponProfile & profile);

};

#endif