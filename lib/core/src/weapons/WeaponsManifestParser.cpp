#include "core/weapons/WeaponsManifestParser.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

struct IDebug;
using json = nlohmann::json;

namespace {

std::string requireString(const json& j, const char* key, IDebug* debug) {
    if (!j.contains(key) || !j.at(key).is_string()) {
        debug->error(std::string("missing or invalid string field '") + key + "'");
    }
    return j.at(key).get<std::string>();
}
}

WeaponsManifest WeaponsManifestParser::parseFromText(const std::string& jsonText, IDebug* debug) {
    json root = json::parse(jsonText);

    if (!root.is_object()) {
        debug->error("root must be an object");
    }

    WeaponsManifest manifest;

    if (root.contains("version")) {
        if (!root.at("version").is_number_integer()) {
            debug->error("field 'version' must be an integer");
        }
        manifest.version = root.at("version").get<int>();
    }

    if (!root.contains("banks") || !root.at("banks").is_array()) {
        debug->error("missing or invalid 'banks'");
    }

    for (const auto& bankJson : root.at("banks")) {
        if (!bankJson.is_object()) {
            debug->error("bank must be an object");
        }
        
        WeaponBankManifest bank;
        bank.name = requireString(bankJson, "name", debug);
        debug->log("Parsing weapon bank " + bank.name);

        if (!bankJson.contains("weapons") || !bankJson.at("weapons").is_array()) {
            debug->error("bank '" + bank.name + "' missing or invalid 'weapons'");
        }
        if (bankJson.at("weapons").empty()) {
            debug->error("bank '" + bank.name + "' must contain at least one weapon");
        }
        
        for (const auto& weaponJson : bankJson.at("weapons")) {
            if (!weaponJson.is_string()) {
                debug->error("bank '" + bank.name + "' contains non-string weapon entry");
            }
            debug->log("Parsing weapon entry: " + weaponJson.get<std::string>());
            bank.weapons.push_back(weaponJson.get<std::string>());
        }

        manifest.banks.push_back(std::move(bank));
    }

    return manifest;
}
