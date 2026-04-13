//
// Created by user on 4/13/2026.
//

#ifndef BLASTER_FIRMWARE_BASICWEAPONPROFILE_H
#define BLASTER_FIRMWARE_BASICWEAPONPROFILE_H

#include "core/weapons/WeaponProfile.h"
#include "nlohmann/json.hpp"


struct BasicWeaponProfile : WeaponProfile {
    std::string name;
    std::string file;
    std::string category;

    int numRounds = 0;           // <= 0 means infinite ammo

    std::string boltLockFile;    // plays on last shot
    std::string emptyFile;       // plays on empty trigger pull
    std::string reloadFile;      // plays on reload

    static bool from_json(const nlohmann::json& j, BasicWeaponProfile& out) {
        if (!j.is_object()) {
            return false;
        }

        if (!j.contains("name") || !j["name"].is_string()) {
            return false;
        }

        if (!j.contains("file") || !j["file"].is_string()) {
            return false;
        }

        if (!j.contains("category") || !j["category"].is_string()) {
            return false;
        }

        out.name = j["name"].get<std::string>();
        out.file = j["file"].get<std::string>();
        out.category = j["category"].get<std::string>();

        out.numRounds = 0;
        if (j.contains("num_rounds") && j["num_rounds"].is_number_integer()) {
            out.numRounds = j["num_rounds"].get<int>();
        }

        out.boltLockFile.clear();
        if (j.contains("bolt_lock_file") && j["bolt_lock_file"].is_string()) {
            out.boltLockFile = j["bolt_lock_file"].get<std::string>();
        }

        out.emptyFile.clear();
        if (j.contains("empty_file") && j["empty_file"].is_string()) {
            out.emptyFile = j["empty_file"].get<std::string>();
        }

        out.reloadFile.clear();
        if (j.contains("reload_file") && j["reload_file"].is_string()) {
            out.reloadFile = j["reload_file"].get<std::string>();
        }

        return true;
    }
};


#endif //BLASTER_FIRMWARE_BASICWEAPONPROFILE_H