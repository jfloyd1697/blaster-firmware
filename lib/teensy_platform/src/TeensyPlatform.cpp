#include "platform/teensy/TeensyPlatform.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include <memory>
#include <string>

#include "core/audio/AudioEngine.h"
#include "core/debug/IDebug.h"
#include "display/Ssd1306StatusDisplay.h"

#ifndef BLASTER_OLED_ADDRESS
#define BLASTER_OLED_ADDRESS 0x3C
#endif
#include "core/lights/LightEngine.h"
#include "core/storage/ResourceTreeDumper.h"
#include "platform/teensy/audio/TeensyAudioBackend.h"
#include "platform/teensy/input/TeensyInput.h"
#include "platform/teensy/lights/TeensyLightBackend.h"
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
    services.debug->log("TeensyPlatform: service startup begin");

    services.debug->log("TeensyPlatform: starting time service");
    services.time = std::make_shared<TeensyTime>();
    services.debug->log("TeensyPlatform: time service ready");

    services.debug->log("TeensyPlatform: starting input service");
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
    services.debug->log("TeensyPlatform: input service ready");

    services.debug->log("TeensyPlatform: starting storage service");
    SPI.begin();
    const bool sdOk = SD.begin(kAudioShieldSdCsPin);
    if (sdOk) {
        services.debug->log("TeensyPlatform: Audio Shield SD init OK");
        services.debug->log("TeensyPlatform: storage service ready");
    } else {
        services.debug->error("TeensyPlatform: Audio Shield SD init FAILED");
        services.debug->error("TeensyPlatform: storage service failed");
    }

    services.debug->log("TeensyPlatform: starting audio service");
    auto audio = std::make_shared<AudioEngine>(
        std::make_unique<TeensyAudioBackend>(services.debug.get(), kAudioVolume));
    if (!audio->begin()) {
        services.debug->error("TeensyPlatform: audio service failed");
    } else {
        services.debug->log("TeensyPlatform: audio service ready");
    }
    services.audio = audio;

    services.debug->log("TeensyPlatform: starting lights service");
    auto lights = std::make_shared<LightEngine>(
        std::make_unique<TeensyLightBackend>(),
        services.time.get());
    if (!lights->begin()) {
        services.debug->error("TeensyPlatform: lights service failed");
    } else {
        services.debug->log("TeensyPlatform: lights service ready");
    }
    services.lights = lights;

    services.textLoader = std::make_shared<TeensySdTextResourceLoader>(services.debug.get());
    if (sdOk) {
        ResourceTreeDumper::dump(*services.textLoader, *services.debug);
    }
#if !defined(BLASTER_OLED_ENABLED) || BLASTER_OLED_ENABLED
    // Uses the Audio Shield's existing I2C bus: SDA 18, SCL 19.
    Wire.begin();
    Wire.setClock(400000);
    auto oled = std::make_shared<Ssd1306StatusDisplay>(Wire, BLASTER_OLED_ADDRESS);
    if (oled->begin()) {
        services.display = oled;
        services.debug->log("TeensyPlatform: SSD1306 OLED ready");
    } else {
        services.debug->log("TeensyPlatform: optional OLED not detected");
    }
#endif
    services.debug->log("TeensyPlatform: service startup complete");
    return services;
}
