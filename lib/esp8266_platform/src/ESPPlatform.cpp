#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <memory>
#include <string>

#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

#include "platform/esp8266/ESPPlatform.h"
#include "platform/esp8266/input/ESPInput.h"
#include "platform/esp8266/audio/ESPAudioBackendFactory.h"
#include "platform/esp8266/text_resource_loader/ESPTextResourceLoader.h"
#include "platform/esp8266/time/ESPTime.h"

namespace {
    //
    // ------------------------- ESP Debug -------------------------
    //
    struct ESPDebug : public IDebug {
        void log(const std::string &msg) override {
            Serial.println(("[LOG] " + msg).c_str());
        }

        void error(const std::string &msg) override {
            Serial.println(("[ERROR] " + msg).c_str());
        }
    };

} // anonymous namespace


//
// ------------------------- Platform Factory -------------------------
//
PlatformServices ESPPlatformFactory::create() {
    PlatformServices services;

    // Serial
    Serial.begin(115200);
    delay(300);

    // Debug
    services.debug = std::make_unique<ESPDebug>();

    // Time
    services.time = std::make_unique<ESPTime>();

    // Input
    auto input = std::make_unique<ESPInput>(
        ESPInput::PinConfig{
            .trigger = D0,
            .ladder = A0,
            .quit = -1,
        },
        services.time.get()
    );

    input->begin();
    services.input = std::move(input);

    // SPI + SD
    SPI.begin();

    constexpr int sdCsPin = D1;
    bool sdOk = SD.begin(sdCsPin);

    // if (sdOk) {
        // services.debug->log("ESPPlatform: SD init FAILED");
    // } else {
        // services.debug->log("ESPPlatform: SD init OK");
    // }

    // Audio
    auto audio = createESPAudioEngine(
        ESPAudioBackendType::Basic, // or Overlap / Basic
        services.debug.get(),
        services.time.get(),
        2, // voices (keep low for ESP8266)
        30 // overlap ms (for pseudo overlap)
    );

    if (!audio) {
        services.debug->error("ESPPlatform: failed to create audio engine");
    } else if (sdOk && !audio->begin()) {
        services.debug->error("ESPPlatform: audio begin failed");
    }

    services.audio = std::move(audio);

    // Weapon loader
    services.loader = std::make_unique<EspSdTextResourceLoader>(services.debug.get());

    // Asset root
    services.assetRoot = "/assets/";

    return services;
}
