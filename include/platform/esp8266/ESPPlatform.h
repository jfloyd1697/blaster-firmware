#ifndef BLASTER_ESPPLATFORM_H
#define BLASTER_ESPPLATFORM_H

#pragma once

#include <memory>

#include "Platform.h"
#include "platform/esp8266/ESPInput.h"
#include "platform/esp8266/ESPAudioEngine.h"

// Include the ones we just made
#include "IDebug.h"
#include "ITime.h"

// ---------------- Debug ----------------
struct ESPDebug final : public IDebug {
    void log(const std::string& msg) override {
        Serial.print("[LOG] ");
        Serial.println(msg.c_str());
    }

    void error(const std::string& msg) override {
        Serial.print("[ERROR] ");
        Serial.println(msg.c_str());
    }
};

// ---------------- Time ----------------
struct ESPTime final : public ITime {
    uint64_t millis() const override {
        return static_cast<uint64_t>(::millis());
    }
};

// ---------------- Factory ----------------
struct ESPPlatformFactory {
    static PlatformServices create() {
        PlatformServices services;

        services.debug = std::make_unique<ESPDebug>();

        auto input = std::make_unique<ESPInput>(ESPInput::PinConfig{
            .trigger = D2,
            .nextWeapon = D3,
            .previousWeapon = D0,
            .quit = -1
        });
        input->begin();
        services.input = std::move(input);

        services.time = std::make_unique<ESPTime>();

        auto audio = std::make_unique<ESPAudioEngine>(services.debug.get());
        audio->begin(D1); // SD CS
        services.audio = std::move(audio);

        services.lights = nullptr;
        services.assetRoot = "/";

        return services;
    }
};
#endif