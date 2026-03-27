//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_WEAPONJSONLOADER_H
#define BLASTER_WEAPONJSONLOADER_H


#pragma once
#include <string>
#include <vector>

// Forward declare WeaponProfile to avoid circular dependency
struct WeaponProfile;

// Function to load all weapon profiles from a JSON file
std::vector<WeaponProfile> loadWeaponProfiles(const std::string& jsonFilePath);

#endif //BLASTER_WEAPONJSONLOADER_H