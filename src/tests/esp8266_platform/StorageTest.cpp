#include <Arduino.h>

#include "../../../../lib/esp8266_platform/include/ESPPlatform.h"
#include "../../../lib/core/weapons/IWeaponLoader.h"

namespace {
    PlatformServices services;

    bool runStorageTest() {
        services.debug->log("=== ESP Storage Test (Platform) ===");

        const std::string jsonPath = services.assetRoot + "weapon_profiles.json";

        if (!SD.exists(jsonPath.c_str())) {
            services.debug->error("Missing JSON: " + jsonPath);
            return false;
        }
        services.debug->log("Loading: " + jsonPath);

        auto banks = services.loader -> loadSoundBanks(jsonPath);

        if (banks.empty()) {
            services.debug->error("No sound banks loaded!");
            return false;
        }

        services.debug->log("Loaded " + std::to_string(banks.size()) + " banks");

        for (const auto& bank : banks) {
            services.debug->log("Bank: " + bank.name);

            for (const auto& weapon : bank.weapons) {
                services.debug->log("  Weapon: " + weapon.name);

                const std::string file = services.assetRoot + weapon.file;

                if (!SD.exists(file.c_str())) {
                    services.debug->error("    MISSING: " + file);
                } else {
                    services.debug->log("    OK: " + file);
                }

                // Optional files
                if (!weapon.boltLockFile.empty()) {
                    std::string f = services.assetRoot + weapon.boltLockFile;
                    if (!SD.exists(f.c_str())) {
                        services.debug->error("    MISSING boltLock: " + f);
                    }
                }

                if (!weapon.emptyFile.empty()) {
                    std::string f = services.assetRoot + weapon.emptyFile;
                    if (!SD.exists(f.c_str())) {
                        services.debug->error("    MISSING empty: " + f);
                    }
                }

                if (!weapon.reloadFile.empty()) {
                    std::string f = services.assetRoot + weapon.reloadFile;
                    if (!SD.exists(f.c_str())) {
                        services.debug->error("    MISSING reload: " + f);
                    }
                }
            }
        }

        services.debug->log("Storage test complete");
        return true;
    }
}

void setup() {
    services = ESPPlatformFactory::create();

    runStorageTest();
}

void loop() {
}