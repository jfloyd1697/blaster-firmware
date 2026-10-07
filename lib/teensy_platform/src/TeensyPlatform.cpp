#include "platform/teensy/TeensyPlatform.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include <memory>
#include <string>

#include "audio/AudioEngine.h"
#include "core/debug/IDebug.h"
#include "platform/teensy/audio/TeensyAudioBackend.h"
#include "platform/teensy/input/TeensyInput.h"
#include "platform/teensy/lights/TeensyLights.h"
#include "platform/teensy/text_resource_loader/TeensySdTextResourceLoader.h"
#include "platform/teensy/time/TeensyTime.h"

namespace {
constexpr int kAudioShieldSdCsPin = 10;
constexpr int kTriggerPin = 0;
constexpr int kButtonLadderPin = A0;
constexpr float kAudioVolume = 0.65F;

struct TeensyDebug : IDebug {
    void log(const std::string& msg) override {
        Serial.println(("[LOG] " + msg).c_str());
    }

    void error(const std::string& msg) override {
        Serial.println(("[ERROR] " + msg).c_str());
    }
};
}

PlatformServices TeensyPlatformFactory::create() {
    PlatformServices services;

    Serial.begin(115200);

    services.debug = std::make_shared<TeensyDebug>();
    services.time = std::make_shared<TeensyTime>();

    auto input = std::make_shared<TeensyInput>(
        TeensyInput::PinConfig{
            .trigger = kTriggerPin,
            .ladder = kButtonLadderPin,
            .quit = -1,
            .triggerActiveLow = true,
            .quitActiveLow = true,
        },
        services.time.get());
    input->begin();
    services.input = input;

    SPI.begin();
    const bool sdOk = SD.begin(kAudioShieldSdCsPin);
    if (sdOk) {
        services.debug->log("TeensyPlatform: Audio Shield SD init OK");
    } else {
        services.debug->error("TeensyPlatform: Audio Shield SD init FAILED");
    }

    auto audio = std::make_shared<AudioEngine>(
        std::make_unique<TeensyAudioBackend>(services.debug.get(), kAudioVolume));
    if (!audio->begin()) {
        services.debug->error("TeensyPlatform: audio begin failed");
    }
    services.audio = audio;

    auto lights = std::make_shared<TeensyLights>();
    if (!lights->begin()) {
        services.debug->error("TeensyPlatform: lights begin failed");
    }
    services.lights = lights;

    services.textLoader = std::make_shared<TeensySdTextResourceLoader>(services.debug.get());
    return services;
}
