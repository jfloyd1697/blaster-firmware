#include <memory>
#include <string>

#include "core/Blaster.h"
#include "core/FallbackMode.h"
#include "core/Platform.h"
#include "core/audio/IAudioEngine.h"
#include "core/assets/AssetLayoutValidator.h"
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
    std::unique_ptr<FallbackMode> fallbackMode;
    bool appInitialized = false;

    bool initializeApp() {
        if (!services.debug || !services.textLoader) {
            return false;
        }

        services.debug->log("App initialization starting");

        const std::string weaponsManifestPath = AssetLayoutValidator::DefaultManifestPath;

        services.debug->log("App: validating asset layout");
        const AssetLayoutReport assetReport = AssetLayoutValidator::validate(
            *services.textLoader,
            services.debug.get(),
            weaponsManifestPath,
            AssetValidationDepth::ReferencesOnly);
        services.debug->log("App: asset layout validation complete");
        if (!assetReport.ok) {
            services.debug->error(
                "Asset layout validation failed with " + std::to_string(assetReport.issues.size()) + " issue(s)");
            for (const auto& issue : assetReport.issues) {
                services.debug->error(issue.path + ": " + issue.message);
            }
            fallbackMode = std::make_unique<FallbackMode>(services);
            fallbackMode->initialize();
            return true;
        }

        services.debug->log(
            "Asset layout valid: " + std::to_string(assetReport.bankCount) + " banks, " +
            std::to_string(assetReport.weaponCount) + " weapons, " +
            std::to_string(assetReport.soundFileCount) + " audio files, " +
            std::to_string(assetReport.lightSequenceCount) + " light sequences");

        services.debug->log("App: loading weapon banks");
        auto banks = WeaponLoader::loadBanks(*services.textLoader, *services.debug, weaponsManifestPath);
        services.debug->log("App: weapon bank loading complete");
        if (banks.empty()) {
            services.debug->error("No sound banks found; starting fallback mode");
            fallbackMode = std::make_unique<FallbackMode>(services);
            fallbackMode->initialize();
            return true;
        }

        services.debug->log("Loaded bank count: " + std::to_string(banks.size()));

        services.debug->log("App: constructing blaster");
        blaster = std::make_unique<Blaster>(services, banks);
        services.debug->log("App: blaster constructed");

        services.debug->log("App initialization complete");
        return true;
    }

    bool tickApp() {
        if (services.input) {
            services.input->update();
        }

        if (services.audio) {
            services.audio->update();
        }

        if (services.lights) {
            services.lights->update();
        }

        if (blaster) {
            return blaster->update();
        }
        if (fallbackMode) {
            return fallbackMode->update();
        }

        if (services.debug) {
            services.debug->error("tickApp: neither blaster nor fallback mode is initialized");
        }
        return false;
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
