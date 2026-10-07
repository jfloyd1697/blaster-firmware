#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <memory>
#include <string>

#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

#include "platform/esp8266/ESPPlatform.h"

#include "audio/AudioEngine.h"
#include "platform/esp8266/audio/BasicESPAudioBackend.h"
#include "platform/esp8266/input/ESPInput.h"
#include "platform/esp8266/text_resource_loader/ESPTextResourceLoader.h"
#include "platform/esp8266/time/ESPTime.h"


#define LOG_RED   "\x1b[31m"
#define LOG_YELLOW "\x1b[33m"
#define LOG_RESET "\x1b[0m"

namespace {
    //
    // ------------------------- ESP Debug -------------------------
    //
    struct ESPDebug : IDebug {
        void log(const std::string &msg) override {
            Serial.println(("[LOG] " + msg).c_str());
        }

        void error(const std::string &msg) override {
            Serial.print("\x1b[31m");
            Serial.print("[ERROR] ");
            Serial.print(msg);
            Serial.println("\x1b[0m");
            Serial.println((LOG_RED + "[ERROR] " + msg + LOG_RESET).c_str());
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
    const bool sdOk = SD.begin(sdCsPin);

    services.audio = std::make_shared<AudioEngine>(
        std::make_unique<BasicESPAudioBackend>(services.debug.get())
    );

    services.audio->begin();

    services.textLoader = std::make_unique<EspSdTextResourceLoader>(services.debug.get());

    if (sdOk) {
        services.debug->log("ESPPlatform: SD init OK");
    } else {
        services.debug->log("ESPPlatform: SD init FAILED");
    }

    return services;
}
