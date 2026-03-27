#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct WeaponProfile {
    std::string name;
    std::string file;

    // Optional: helper to load from json object
    static WeaponProfile from_json(const nlohmann::json& j) {
        WeaponProfile profile;
        profile.name = j.at("name").get<std::string>();
        profile.file = j.at("file").get<std::string>();
        return profile;
    }
};