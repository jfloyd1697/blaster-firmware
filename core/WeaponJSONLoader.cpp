#include "WeaponJSONLoader.h"
#include "WeaponProfile.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

std::vector<WeaponProfile> loadWeaponProfiles(const std::string& jsonFilePath) {
    std::vector<WeaponProfile> profiles;

    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open weapon JSON file: " << jsonFilePath << std::endl;
        return profiles;
    }

    nlohmann::json j;
    try {
        file >> j;
        for (auto& item : j) {
            profiles.push_back(WeaponProfile::from_json(item));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }

    return profiles;
}