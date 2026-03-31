#include <memory>
#include <string>
#include <vector>

#ifdef PLATFORM_ESP
#include <Arduino.h>
#endif

#include "Blaster.h"
#include "Platform.h"
#include "weapons/WeaponJSONLoader.h"

#ifdef PLATFORM_PC
#include "platform/pc/PCPlatform.h"
#endif

#ifdef PLATFORM_ESP
#include "platform/esp8266/ESPPlatform.h"
#endif

namespace {
        PlatformServices services;
        std::unique_ptr<Blaster> blaster;

        bool initializeApp() {
                services.debug->log("App initialization starting");

                const std::string weaponJsonPath = services.assetRoot + "weapon_profiles.json";
                auto banks = loadSoundBanks(weaponJsonPath);

                if (banks.empty()) {
                        services.debug->error("No sound banks found!");
                        return false;
                }

                services.debug->log("Loaded bank count: " + std::to_string(banks.size()));

                blaster = std::make_unique<Blaster>(services, banks);

                services.debug->log("App initialization complete");
                return true;
        }

        bool tickApp() {
                if (blaster) {
                        return blaster->update();
                }

                return true;
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
        Serial.begin(115200);
        delay(200);

        services = ESPPlatformFactory::create();
        initializeApp();
}

void loop() {
        tickApp();
}

#endif
