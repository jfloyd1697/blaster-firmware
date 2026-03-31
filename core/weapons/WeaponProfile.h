#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct WeaponProfile {
    std::string name;
    std::string file;
    std::string category;

    static WeaponProfile from_json(const nlohmann::json& j) {
        WeaponProfile profile;
        profile.name = j.at("name").get<std::string>();
        profile.file = j.at("file").get<std::string>();
        profile.category = j.at("category").get<std::string>();
        return profile;
    }
};


struct SoundBank {
    std::string name;
    std::vector<WeaponProfile> weapons;
};