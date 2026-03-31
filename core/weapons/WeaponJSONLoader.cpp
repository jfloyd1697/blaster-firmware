#include "WeaponJSONLoader.h"

#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

std::vector<SoundBank> loadSoundBanks(const std::string& jsonFilePath) {
    std::vector<SoundBank> banks;

    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open weapon JSON file: " << jsonFilePath << std::endl;
        return banks;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!j.is_array()) {
            std::cerr << "Expected top-level JSON array in: " << jsonFilePath << std::endl;
            return banks;
        }

        // Group by category
        std::map<std::string, std::vector<WeaponProfile>> groupedProfiles;

        for (const auto& item : j) {
            WeaponProfile profile = WeaponProfile::from_json(item);
            groupedProfiles[profile.category].push_back(std::move(profile));
        }

        // Convert grouped map into SoundBank vector
        for (auto& [category, weapons] : groupedProfiles) {
            SoundBank bank;
            bank.name = category;
            bank.weapons = std::move(weapons);
            banks.push_back(std::move(bank));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }

    return banks;
}