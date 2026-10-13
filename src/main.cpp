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
#include <Arduino.h>
#endif

namespace {
    PlatformServices services;
    std::unique_ptr<Blaster> blaster;
    std::unique_ptr<FallbackMode> fallbackMode;
    bool appInitialized = false;
#ifdef PLATFORM_STM32F411
    enum class BridgeMode { Normal, RemoteInput, DirectControl };
    BridgeMode bridgeMode = BridgeMode::Normal;
    std::uint32_t lastBridgeHeartbeat = 0;
    String bridgeLine;
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

    void bridgePoll();

    void runControl() {
        const std::uint32_t start = micros();
        if (services.input) services.input->update();
        if (blaster && bridgeMode != BridgeMode::DirectControl) {
            blaster->update();
        } else if (fallbackMode) {
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
        if (blaster && blaster->hasPendingWeaponLoad()) {
            const std::uint32_t before = micros();
            blaster->servicePendingWeaponLoad();
            const std::uint32_t elapsed = micros() - before;
            if (elapsed > maximumWeaponLoadUs) maximumWeaponLoadUs = elapsed;
            ++weaponLoads;
            return;
        }
        if (auto* lights = static_cast<LightEngine*>(services.lights.get())) {
            const std::uint32_t before = micros();
            lights->prefetchNextFrame();
            const std::uint32_t elapsed = micros() - before;
            if (elapsed > maximumForegroundBlockUs) maximumForegroundBlockUs = elapsed;
            ++backgroundReadCount;
        }
        if (controlSchedule.slack(micros(), kControlPeriodUs) >= kBackgroundMinimumSlackUs) bridgePoll();
        // Lowest-priority, bounded I2C transfer. At most 8 display-data bytes
        // per pass; do not update the OLED in the control tick or an ISR.
        if (services.display && controlSchedule.slack(micros(), kControlPeriodUs) >= kDisplayMinimumSlackUs) {
            services.display->service(millis());
        }
    }

    void bridgeReply(const String& message) {
        Serial.print("@BRIDGE ");
        Serial.println(message);
    }

    void bridgeSetMode(const BridgeMode next) {
        if (services.audio) services.audio->stop();
        if (services.lights) services.lights->stopSequence();
        if (services.input) services.input->setRemoteInputEnabled(next == BridgeMode::RemoteInput);
        bridgeMode = next;
        if (services.display) {
            const DisplayControlMode mode = next == BridgeMode::RemoteInput
                ? DisplayControlMode::RemoteInput
                : next == BridgeMode::DirectControl ? DisplayControlMode::DirectControl
                    : DisplayControlMode::Normal;
            services.display->setControlMode(mode);
        }
        lastBridgeHeartbeat = millis();
        bridgeReply("OK MODE");
    }

    void bridgeCommand(const String& line) {
        if (line == "HELLO") {
            bridgeReply("HELLO 1 STM32F411 INPUT DIRECT");
        } else if (line == "MODE NORMAL") {
            bridgeSetMode(BridgeMode::Normal);
        } else if (line == "MODE INPUT") {
            bridgeSetMode(BridgeMode::RemoteInput);
        } else if (line == "MODE DIRECT") {
            bridgeSetMode(BridgeMode::DirectControl);
        } else if (line == "STATS") {
            bridgeReply("STATS control_ticks=" + String(controlSchedule.ticks()) +
                " missed_ticks=" + String(controlSchedule.missedTicks()) +
                " max_lateness_us=" + String(controlSchedule.maximumLatenessUs()) +
                " max_control_us=" + String(maximumControlExecutionUs) +
                " max_background_us=" + String(maximumForegroundBlockUs) +
                " max_audio_us=" + String(maximumAudioServiceUs) +
                " max_weapon_load_us=" + String(maximumWeaponLoadUs) +
                " weapon_loads=" + String(weaponLoads) +
                " background_passes=" + String(backgroundReadCount) +
                " light_misses=" + String(services.lights ?
                    static_cast<LightEngine*>(services.lights.get())->missedPrefetchCount() : 0));
        } else if (line == "PING") {
            lastBridgeHeartbeat = millis();
            bridgeReply("PONG");
        } else if (line.startsWith("INPUT ") && bridgeMode == BridgeMode::RemoteInput) {
            const int separator = line.indexOf(' ', 6);
            if (separator < 0 || !services.input) { bridgeReply("ERR INPUT"); return; }
            const String name = line.substring(6, separator);
            const String value = line.substring(separator + 1);
            ButtonID id;
            if (name == "TRIGGER") id = ButtonID::Trigger;
            else if (name == "RELOAD") id = ButtonID::Reload;
            else if (name == "NEXT") id = ButtonID::NextWeapon;
            else if (name == "PREVIOUS") id = ButtonID::PreviousWeapon;
            else { bridgeReply("ERR INPUT"); return; }
            if (value != "0" && value != "1") { bridgeReply("ERR VALUE"); return; }
            services.input->setRemoteButton(id, value == "1");
            lastBridgeHeartbeat = millis();
            bridgeReply("OK INPUT");
        } else if (line == "STOP" && bridgeMode == BridgeMode::DirectControl) {
            if (services.audio) services.audio->stop();
            if (services.lights) services.lights->stopSequence();
            bridgeReply("OK STOP");
        } else if (line.startsWith("AUDIO ") && bridgeMode == BridgeMode::DirectControl) {
            if (services.audio) services.audio->playSound(std::string(line.substring(6).c_str()), false, false);
            bridgeReply("OK AUDIO");
        } else if (line.startsWith("LIGHT ") && bridgeMode == BridgeMode::DirectControl) {
            const bool ok = services.lights && services.textLoader &&
                services.lights->playSequenceFile(std::string(line.substring(6).c_str()), *services.textLoader, LightSequencePlayback{});
            bridgeReply(ok ? "OK LIGHT" : "ERR LIGHT");
        } else {
            bridgeReply("ERR COMMAND_OR_MODE");
        }
    }

    void bridgePoll() {
        // A busy USB host must not monopolize the control loop.
        std::size_t bytesProcessed = 0;
        while (Serial.available() && bytesProcessed++ < 48u) {
            const char ch = static_cast<char>(Serial.read());
            if (ch == '\n') {
                if (bridgeLine.length() && bridgeLine[bridgeLine.length() - 1] == '\r')
                    bridgeLine.remove(bridgeLine.length() - 1);
                if (bridgeLine.length()) bridgeCommand(bridgeLine);
                bridgeLine = "";
            } else if (bridgeLine.length() < 240 && ch >= 32 && ch <= 126) {
                bridgeLine += ch;
            } else if (ch != '\r') {
                bridgeLine = "";
            }
        }
        if (bridgeMode != BridgeMode::Normal && millis() - lastBridgeHeartbeat > 1500) {
            bridgeSetMode(BridgeMode::Normal);
            bridgeReply("TIMEOUT");
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
