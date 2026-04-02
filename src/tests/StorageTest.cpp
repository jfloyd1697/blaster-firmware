#include <Arduino.h>

#include "platform/esp8266//ESPPlatform.h"
#include "core/weapons/IWeaponLoader.h"

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