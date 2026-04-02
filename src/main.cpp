#include <memory>
#include <string>

#include "core/Blaster.h"
#include "core/Platform.h"
#include "platform/pc/PCPlatform.h"

// #ifdef PLATFORM_PC
// #endif
//
// #ifdef PLATFORM_ESP
// #include "platform/esp8266/ESPPlatform.h"
// #include <Arduino.h>
// #endif

namespace {
    PlatformServices services;
    std::unique_ptr<Blaster> blaster;
    bool appInitialized = false;

    bool initializeApp() {
        if (!services.debug || !services.loader) {
            return false;
        }

        services.debug->log("App initialization starting");

        const std::string weaponJsonPath = services.assetRoot + "weapons/chainsaw/weapon_profile.json";
        auto banks = services.loadSoundBanks(weaponJsonPath);

        // if (banks.empty()) {
        //     services.debug->error("No sound banks found");
        //     return false;
        // }

        services.debug->log("Loaded bank count: " + std::to_string(banks.size()));

        blaster = std::make_unique<Blaster>(services, banks);

        services.debug->log("App initialization complete");
        return true;
    }

    bool tickApp() {
        if (!blaster) {
            if (services.debug) {
                services.debug->error("tickApp: blaster not initialized");
            }
            return false;
        }

        if (services.input) {
            services.input->update();
        }

        if (services.audio) {
            services.audio->update();
        }

        if (services.lights) {
            services.lights->update();
        }

        return blaster->update();
    }
}

// #ifdef PLATFORM_PC

int main() {
    services = PCPlatformFactory::create();

    if (!initializeApp()) {
        return 1;
    }

    while (tickApp()) {
    }

    return 0;
}

// #endif

// #ifdef PLATFORM_ESP
//
// void setup() {
//     services = ESPPlatformFactory::create();
//     appInitialized = initializeApp();
//
//     if (!appInitialized && services.debug) {
//         services.debug->error("setup: app initialization failed");
//     }
// }
//
// void loop() {
//     if (!appInitialized) {
//         delay(100);
//         return;
//     }
//
//     if (!tickApp()) {
//         delay(1);
//     }
// }
//
// #endif
