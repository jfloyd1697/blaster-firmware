#include "platform/stm32f411/Stm32F411Platform.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include <memory>
#include <string>

#include "audio/AudioEngine.h"
#include "core/debug/IDebug.h"
#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"
#include "platform/stm32f411/input/Stm32F411Input.h"
#include "platform/stm32f411/lights/Stm32F411Lights.h"
#include "platform/stm32f411/text_resource_loader/Stm32F411SdTextResourceLoader.h"
#include "platform/stm32f411/time/Stm32F411Time.h"

#ifndef BLASTER_STM32F411_LED_COUNT
#define BLASTER_STM32F411_LED_COUNT 1
#endif

namespace {
struct Stm32F411Debug : IDebug {
    void log(const std::string& msg) override {
        Serial.println(("[LOG] " + msg).c_str());
    }

    void error(const std::string& msg) override {
        Serial.println(("[ERROR] " + msg).c_str());
    }
};

bool initializeSd(const Stm32F411BoardConfig& board, IDebug* debug) {
    SPI.setMOSI(board.sd.mosi);
    SPI.setMISO(board.sd.miso);
    SPI.setSCLK(board.sd.sck);
    SPI.begin();

    const bool ok = SD.begin(board.sd.cs);
    if (debug != nullptr) {
        if (ok) {
            debug->log("Stm32F411Platform: external SD init OK");
        } else {
            debug->error("Stm32F411Platform: external SD init FAILED");
        }
    }
    return ok;
}
}

PlatformServices Stm32F411PlatformFactory::create(const Stm32F411BoardConfig& board) {
    PlatformServices services;

    Serial.begin(115200);
    services.debug = std::make_shared<Stm32F411Debug>();
    services.time = std::make_shared<Stm32F411Time>();

    if (board.statusLed.has_value()) {
        pinMode(*board.statusLed, OUTPUT);
        digitalWrite(*board.statusLed, board.statusLedActiveLow ? HIGH : LOW);
    }

    services.debug->log(std::string("Stm32F411Platform: board ") + board.name);
    initializeSd(board, services.debug.get());

    auto input = std::make_shared<Stm32F411Input>(board.input, services.time.get());
    input->begin();
    services.input = input;

    auto audio = std::make_shared<AudioEngine>(
        std::make_unique<Stm32F411AudioBackend>(board.audio, services.debug.get()));
    if (!audio->begin()) {
        services.debug->error("Stm32F411Platform: audio begin failed");
    }
    services.audio = audio;

    auto lights = std::make_shared<Stm32F411Lights>(board.lights);
    if (!lights->begin()) {
        services.debug->error("Stm32F411Platform: lights begin failed");
    }
    services.lights = lights;

    services.textLoader = std::make_shared<Stm32F411SdTextResourceLoader>(services.debug.get());
    return services;
}

PlatformServices Stm32F411PlatformFactory::createConfiguredBoard() {
#if defined(BLASTER_STM32F411_BOARD_BLACKPILL)
    return create(Stm32F411Boards::blackPillV20(BLASTER_STM32F411_LED_COUNT));
#elif defined(BLASTER_STM32F411_BOARD_DEVEBOX)
    return create(Stm32F411Boards::devEBox(BLASTER_STM32F411_LED_COUNT));
#else
#error "Select an STM32F411 board configuration"
#endif
}
