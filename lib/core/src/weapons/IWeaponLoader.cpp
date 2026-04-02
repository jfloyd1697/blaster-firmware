#include "core/weapons/IWeaponLoader.h"

#include <nlohmann/json.hpp>

#include "core/debug/IDebug.h"
#include "core/weapons/ShootModeType.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"

using json = nlohmann::json;

namespace {
    ShootModeType parseShootModeType(const std::string& value) {
        if (value == "single") {
            return ShootModeType::Single;
        }

        if (value == "charge") {
            return ShootModeType::Charge;
        }

        if (value == "rapid") {
            return ShootModeType::Rapid;
        }

        if (value == "burst") {
            return ShootModeType::Burst;
        }

        return ShootModeType::Single;
    }

    std::string getStringOr(const json& obj,
                            const char* key,
                            const std::string& fallback = {}) {
        if (!obj.contains(key) || !obj.at(key).is_string()) {
            return fallback;
        }

        return obj.at(key).get<std::string>();
    }

    int getIntOr(const json& obj, const char* key, const int fallback) {
        if (!obj.contains(key) || !obj.at(key).is_number_integer()) {
            return fallback;
        }

        return obj.at(key).get<int>();
    }

    std::uint32_t getUint32Or(const json& obj,
                              const char* key,
                              const std::uint32_t fallback) {
        if (!obj.contains(key) || !obj.at(key).is_number_integer()) {
            return fallback;
        }

        return static_cast<std::uint32_t>(obj.at(key).get<int>());
    }

    WeaponSounds parseWeaponSounds(const json& soundsObj) {
        WeaponSounds sounds;

        if (!soundsObj.is_object()) {
            return sounds;
        }

        sounds.shot = getStringOr(soundsObj, "shot");
        sounds.chargeStart = getStringOr(soundsObj, "chargeStart");
        sounds.chargeLoop = getStringOr(soundsObj, "chargeLoop");
        sounds.chargeRelease = getStringOr(soundsObj, "chargeRelease");
        sounds.reload = getStringOr(soundsObj, "reload");
        sounds.empty = getStringOr(soundsObj, "empty");

        return sounds;
    }

    ShootModeConfig parseShootModeConfig(const json& modeObj) {
        ShootModeConfig config;

        if (!modeObj.is_object()) {
            return config;
        }

        config.type = parseShootModeType(getStringOr(modeObj, "type", "single"));
        config.cooldownMs = getUint32Or(modeObj, "cooldownMs", 100);
        if (modeObj.contains("burstCount") && modeObj.at("burstCount").is_number_integer()) {
            config.burstCount = getIntOr(modeObj, "burstCount", 3);
            config.burstIntervalMs = getUint32Or(modeObj, "burstIntervalMs", 60);
        }
        if (modeObj.contains("charge") && modeObj.at("charge").is_object()) {
             const json& chargeObj = modeObj.at("charge");
             config.minChargeMs = getUint32Or(chargeObj, "minChargeMs", 300);
             config.maxChargeMs = getUint32Or(chargeObj, "maxChargeMs", 1200);
        }

        return config;
    }

    WeaponProfile parseWeaponProfile(const json& weaponObj) {
        WeaponProfile profile;

        if (!weaponObj.is_object()) {
            return profile;
        }

        profile.name = getStringOr(weaponObj, "name");
        if (weaponObj.contains("ammoPerShot") && weaponObj.at("ammoPerShot").is_number_integer()) {
            profile.ammoPerShot = getIntOr(weaponObj, "ammoPerShot", 0);
            profile.magazineSize = getIntOr(weaponObj, "magazineSize", 0);
        }

        if (weaponObj.contains("sounds")) {
            profile.sounds = parseWeaponSounds(weaponObj.at("sounds"));
        }

        if (weaponObj.contains("shootMode")) {
            profile.shootMode = parseShootModeConfig(weaponObj.at("shootMode"));
        }

        return profile;
    }

    SoundBank parseSoundBank(const json& bankObj) {
        SoundBank bank;

        if (!bankObj.is_object()) {
            return bank;
        }

        bank.name = getStringOr(bankObj, "name");

        if (bankObj.contains("weapons") && bankObj.at("weapons").is_array()) {
            for (const auto& weaponObj : bankObj.at("weapons")) {
                bank.weapons.push_back(parseWeaponProfile(weaponObj));
            }
        }

        return bank;
    }
}

std::vector<SoundBank> IWeaponLoader::loadSoundBanks(const std::string& jsonFilePath, IDebug* debug) {

    const std::string text = loadText(jsonFilePath, debug);
    if (text.empty()) {
        return {};
     }

    json root;
    try {
        root = json::parse(text);
    } catch (const json::parse_error&) {
        return {};
    }

    std::vector<SoundBank> banks = {};
    if (root.is_object() && root.contains("banks") && root.at("banks").is_array()) {
        for (const auto& bankObj : root.at("banks")) {
            banks.push_back(parseSoundBank(bankObj));
        }
    } else if (root.is_array()) {
        for (const auto& bankObj : root) {
            banks.push_back(parseSoundBank(bankObj));
        }
    }

    return banks;
}