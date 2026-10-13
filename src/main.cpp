#include <memory>
#include <string>
#include <cstdint>

#include "core/Blaster.h"
#include "core/FallbackMode.h"
#include "core/Platform.h"
#include "core/display/IStatusDisplay.h"
#include "core/audio/IAudioEngine.h"
#include "core/assets/AssetLayoutValidator.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/time/ITime.h"
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
#include "core/lights/LightEngine.h"
#include "realtime/ControlSchedule.h"
#include "Stm32Bridge.h"
#include <Arduino.h>
#endif

namespace {
    PlatformServices services;
    std::unique_ptr<Blaster> blaster;
    std::unique_ptr<FallbackMode> fallbackMode;
    bool appInitialized = false;
#ifdef PLATFORM_STM32F411
    std::unique_ptr<Stm32Bridge> hardwareBridge;
    realtime::ControlSchedule controlSchedule;
    std::uint32_t maximumControlExecutionUs = 0;
    std::uint32_t maximumForegroundBlockUs = 0;
    std::uint32_t backgroundReadCount = 0;
    std::uint32_t maximumAudioServiceUs = 0;
    std::uint32_t maximumWeaponLoadUs = 0;
    std::uint32_t weaponLoads = 0;
    constexpr std::uint32_t kControlPeriodUs = 1000;
    constexpr std::uint32_t kBackgroundMinimumSlackUs = 400;
    constexpr std::uint32_t kDisplayMinimumSlackUs = 500;

    bool initializeApp();

    void runControl() {
        const std::uint32_t start = micros();
        if (services.input) services.input->update();
        if (blaster && hardwareBridge && !hardwareBridge->directControl() && !hardwareBridge->maintenanceActive()) {
            blaster->update();
        } else if (fallbackMode && hardwareBridge && !hardwareBridge->maintenanceActive()) {
            fallbackMode->update();
        }
        const std::uint32_t executionUs = micros() - start;
        if (executionUs > maximumControlExecutionUs) maximumControlExecutionUs = executionUs;
    }

    void runStm32Foreground() {
        const std::uint32_t now = micros();
        if (controlSchedule.due(now, kControlPeriodUs)) runControl();

        // Rendering and DMA service do not perform lighting file reads.
        if (services.lights) services.lights->update();

        // Single foreground storage service. Priorities: audio, weapon load,
        // lighting prefetch, serial. Only one lower-priority operation per pass.
        // SdFat remains synchronous: the admission check cannot preempt a read.
        if (services.audio) {
            const std::uint32_t before = micros();
            services.audio->update();
            const std::uint32_t elapsed = micros() - before;
            if (elapsed > maximumAudioServiceUs) maximumAudioServiceUs = elapsed;
        }
        if (controlSchedule.slack(micros(), kControlPeriodUs) < kBackgroundMinimumSlackUs) return;
        if (blaster && hardwareBridge && !hardwareBridge->maintenanceActive() &&
            blaster->hasPendingWeaponLoad()) {
            const std::uint32_t before = micros();
            blaster->servicePendingWeaponLoad();
            const std::uint32_t elapsed = micros() - before;
            if (elapsed > maximumWeaponLoadUs) maximumWeaponLoadUs = elapsed;
            ++weaponLoads;
            return;
        }
        if (hardwareBridge && !hardwareBridge->maintenanceActive()) {
            if (auto* lights = static_cast<LightEngine*>(services.lights.get())) {
                const std::uint32_t before = micros();
                lights->prefetchNextFrame();
                const std::uint32_t elapsed = micros() - before;
                if (elapsed > maximumForegroundBlockUs) maximumForegroundBlockUs = elapsed;
                ++backgroundReadCount;
            }
        }
        if (hardwareBridge && controlSchedule.slack(micros(), kControlPeriodUs) >= kBackgroundMinimumSlackUs) {
            hardwareBridge->service();
            if (hardwareBridge->takeActivate()) {
                // Reload files only after all SD handles have been released.
                blaster.reset();
                fallbackMode.reset();
                appInitialized = initializeApp();
                hardwareBridge->finishedActivation();
                return;
            }
        }
        // Lowest-priority, bounded I2C transfer. At most 8 display-data bytes
        // per pass; do not update the OLED in the control tick or an ISR.
        if (services.display && controlSchedule.slack(micros(), kControlPeriodUs) >= kDisplayMinimumSlackUs) {
            services.display->service(millis());
        }
    }

#endif

    bool initializeApp() {
        if (!services.debug || !services.textLoader) {
            return false;
        }

        services.debug->log("App initialization starting");

        const std::string weaponsManifestPath = AssetLayoutValidator::DefaultManifestPath;

        // Fast startup: only load the small compiled bank index here.
        // Full asset verification is available on demand in platform diagnostic 7.

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

        if (blaster || fallbackMode) {
            const bool running = blaster ? blaster->update() : fallbackMode->update();
            if (services.display && services.time) {
                services.display->service(static_cast<std::uint32_t>(services.time->millis()));
            }
            return running;
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
    hardwareBridge = std::make_unique<Stm32Bridge>(services);
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

    runStm32Foreground();
}

#endif
