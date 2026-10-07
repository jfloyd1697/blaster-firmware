#include <memory>
#include <string>

#include "core/Blaster.h"
#include "core/Platform.h"
#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/weapons/WeaponLoader.h"

#ifdef PLATFORM_PC
#include "platform/pc/PCPlatform.h"
#endif

#ifdef PLATFORM_ESP
#include "platform/esp8266/ESPPlatform.h"
#include <Arduino.h>
#endif

#ifdef PLATFORM_TEENSY
#include "platform/teensy/TeensyPlatform.h"
#include <Arduino.h>
#endif

#ifdef PLATFORM_STM32F411
#include "platform/stm32f411/Stm32F411Platform.h"
#include <Arduino.h>
#endif

namespace {
    PlatformServices services;
    std::unique_ptr<Blaster> blaster;
    bool appInitialized = false;

    bool initializeApp() {
        if (!services.debug || !services.textLoader) {
            return false;
        }

        services.debug->log("App initialization starting");

        const std::string weaponsManifestPath = "assets/weapons/weapons_manifest.json";
        auto banks = WeaponLoader::loadBanks(*services.textLoader, *services.debug, weaponsManifestPath);

        if (banks.empty()) {
            services.debug->error("No sound banks found");
            return false;
        }

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

#ifdef PLATFORM_PC

int main() {
    services = PCPlatformFactory::create();

    if (!initializeApp()) {
        return 1;
    }

    while (tickApp()) {
    }

    return 0;
}

#endif

#ifdef PLATFORM_ESP

void setup() {
    services = ESPPlatformFactory::create();
    appInitialized = initializeApp();

    if (!appInitialized && services.debug) {
        services.debug->error("setup: app initialization failed");
    }
}

void loop() {
    if (!appInitialized) {
        delay(100);
        return;
    }

    if (!tickApp()) {
        delay(1);
    }
}

#endif

#ifdef PLATFORM_TEENSY

void setup() {
    services = TeensyPlatformFactory::create();
    appInitialized = initializeApp();

    if (!appInitialized && services.debug) {
        services.debug->error("setup: app initialization failed");
    }
}

void loop() {
    if (!appInitialized) {
        delay(100);
        return;
    }

    if (!tickApp()) {
        delay(1);
    }
}

#endif

#ifdef PLATFORM_STM32F411

void setup() {
    services = Stm32F411PlatformFactory::createConfiguredBoard();
    appInitialized = initializeApp();

    if (!appInitialized && services.debug) {
        services.debug->error("setup: app initialization failed");
    }
}

void loop() {
    if (!appInitialized) {
        delay(100);
        return;
    }

    if (!tickApp()) {
        delay(1);
    }
}

#endif
