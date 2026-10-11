#include "platform/stm32f411/Stm32F411Platform.h"

#include <Arduino.h>
#include <SPI.h>

#include <memory>
#include <string>

#include "core/audio/AudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/lights/LightEngine.h"
#include "core/storage/ResourceTreeDumper.h"
#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"
#include "platform/stm32f411/input/Stm32F411Input.h"
#include "platform/stm32f411/lights/Stm32F411LightBackend.h"
#include "platform/stm32f411/text_resource_loader/Stm32F411SdTextResourceLoader.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"
#include "platform/stm32f411/time/Stm32F411Time.h"

#ifndef BLASTER_STM32F411_LED_COUNT
#define BLASTER_STM32F411_LED_COUNT 1
#endif

SdFat32 g_stm32Sd;

namespace {
constexpr unsigned long kSerialReadyTimeoutMs = 2000;

void initializeSerial() {
    Serial.begin(115200);

    const unsigned long startedAt = millis();
    while (!Serial && (millis() - startedAt) < kSerialReadyTimeoutMs) {
        delay(1);
    }
}

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

    const SdSpiConfig sdConfig(
        board.sd.cs,
        DEDICATED_SPI,
        SD_SCK_MHZ(18),
        &SPI);
    const bool ok = g_stm32Sd.begin(sdConfig);
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

    initializeSerial();
    services.debug = std::make_shared<Stm32F411Debug>();
    services.debug->log("Stm32F411Platform: service startup begin");

    services.debug->log("Stm32F411Platform: starting time service");
    services.time = std::make_shared<Stm32F411Time>();
    services.debug->log("Stm32F411Platform: time service ready");

    if (board.statusLed.has_value()) {
        pinMode(*board.statusLed, OUTPUT);
        digitalWrite(*board.statusLed, board.statusLedActiveLow ? HIGH : LOW);
    }

    services.debug->log(std::string("Stm32F411Platform: board ") + board.name);

    services.debug->log("Stm32F411Platform: starting storage service");
    const bool sdOk = initializeSd(board, services.debug.get());
    services.textLoader = std::make_shared<Stm32F411SdTextResourceLoader>(services.debug.get());
    if (sdOk) {
        services.debug->log("Stm32F411Platform: storage service ready");
    } else {
        services.debug->error("Stm32F411Platform: storage service failed");
    }
    if (sdOk) {
        ResourceTreeDumper::dump(*services.textLoader, *services.debug);
    }

    services.debug->log("Stm32F411Platform: starting input service");
    auto input = std::make_shared<Stm32F411Input>(board.input, services.time.get());
    input->begin();
    services.input = input;
    services.debug->log("Stm32F411Platform: input service ready");

    services.debug->log("Stm32F411Platform: starting audio service");
    auto audio = std::make_shared<AudioEngine>(
        std::make_unique<Stm32F411AudioBackend>(board.audio, services.debug.get()));
    if (!audio->begin()) {
        services.debug->error("Stm32F411Platform: audio service failed");
    } else {
        services.debug->log("Stm32F411Platform: audio service ready");
    }
    services.audio = audio;

    services.debug->log("Stm32F411Platform: starting lights service");
    auto lights = std::make_shared<LightEngine>(
        std::make_unique<Stm32F411LightBackend>(board.lights, services.debug.get()),
        services.time.get());
    if (!lights->begin()) {
        services.debug->error("Stm32F411Platform: lights service failed");
    } else {
        services.debug->log("Stm32F411Platform: lights service ready");
    }
    services.lights = lights;

    services.debug->log("Stm32F411Platform: service startup complete");
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
