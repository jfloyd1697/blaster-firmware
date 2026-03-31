#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <memory>
#include <string>

#include "IDebug.h"
#include "ITime.h"
#include "weapons/IWeaponLoader.h"

#include "ESPPlatform.h"
#include "input/ESPInput.h"
#include "audio/ESPAudioBackendFactory.h"

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

    //
    // ------------------------- ESP Time -------------------------
    //
    struct ESPTime : public ITime {
        uint64_t millis() const override {
            return ::millis();
        }
    };

    //
    // ------------------------- ESP Weapon Loader -------------------------
    //
    class ESPWeaponLoader : public IWeaponLoader {
    protected:
        std::string loadText(const std::string &path) override {
            File file = SD.open(path.c_str(), FILE_READ);
            if (!file) {
                return {};
            }

            const size_t size = file.size();
            if (size == 0 || size > 20 * 1024) {
                file.close();
                return {};
            }

            std::string text;
            text.resize(size);

            const size_t bytesRead = file.readBytes(text.data(), size);
            file.close();

            if (bytesRead != size) {
                return {};
            }

            return text;
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

    const int sdCsPin = D1;
    const bool sdOk = SD.begin(sdCsPin);

    if (!sdOk) {
        services.debug->error("ESPPlatform: SD init FAILED");
    } else {
        services.debug->log("ESPPlatform: SD init OK");
    }

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
    services.loader = std::make_unique<ESPWeaponLoader>();

    // Asset root
    services.assetRoot = "/assets/";

    return services;
}
