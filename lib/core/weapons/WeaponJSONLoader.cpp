#include "WeaponJSONLoader.h"

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

std::vector<SoundBank> loadSoundBanks(const std::string& jsonFilePath) {
    std::vector<SoundBank> banks;

    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open weapon JSON file: " << jsonFilePath << std::endl;
        return banks;
    }

    nlohmann::json j;
    file >> j;

    if (!j.is_array()) {
        std::cerr << "Expected top-level JSON array in: " << jsonFilePath << std::endl;
        return banks;
    }

    std::unordered_map<std::string, std::size_t> bankIndexByCategory;

    for (const auto& item : j) {
        WeaponProfile profile;
        if (!WeaponProfile::from_json(item, profile)) {
            std::cerr << "Skipping invalid weapon entry in JSON" << std::endl;
            continue;
        }

        auto it = bankIndexByCategory.find(profile.category);
        if (it == bankIndexByCategory.end()) {
            bankIndexByCategory[profile.category] = banks.size();
            banks.push_back(SoundBank{profile.category, {}});
            it = bankIndexByCategory.find(profile.category);
        }

        banks[it->second].weapons.push_back(std::move(profile));
    }

    return banks;
}