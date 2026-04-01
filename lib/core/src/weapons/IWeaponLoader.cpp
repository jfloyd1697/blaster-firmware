#include <string>
#include <unordered_map>

#include "core/weapons/IWeaponLoader.h"
#include <nlohmann/json.hpp>

std::vector<SoundBank> IWeaponLoader::parseSoundBanksJson(const std::string& jsonText) {
    std::vector<SoundBank> banks;

    const auto j = nlohmann::json::parse(jsonText, nullptr, false);
    if (j.is_discarded() || !j.is_array()) {
        return banks;
    }

    std::unordered_map<std::string, std::size_t> bankIndexByCategory;

    for (const auto& item : j) {
        WeaponProfile profile;
        if (!WeaponProfile::from_json(item, profile)) {
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

std::vector<SoundBank> IWeaponLoader::loadSoundBanks(const std::string& jsonFilePath) {
    const std::string jsonText = loadText(jsonFilePath);
    if (jsonText.empty()) {
        return {};
    }

    return parseSoundBanksJson(jsonText);
}