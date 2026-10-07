#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

namespace {
constexpr Stm32F411SpiPins kExternalSdPins{
    .mosi = PB15,
    .miso = PB14,
    .sck = PB13,
    .cs = PB12,
};

constexpr Stm32F411I2sPins kMax98357Pins{
    .ws = PA4,
    .bclk = PC10,
    .data = PC12,
};

constexpr Stm32F411InputPins kInputPins{
    .trigger = PA0,
    .ladder = PA1,
};

constexpr pin_size_t kWs2812Pin = PB6;
}

Stm32F411BoardConfig Stm32F411Boards::blackPillV20(const std::uint16_t ledCount) {
    return Stm32F411BoardConfig{
        .name = "STM32F4x1Cx v2.0+",
        .sd = kExternalSdPins,
        .audio = kMax98357Pins,
        .input = kInputPins,
        .lights = Stm32F411LightPins{.data = kWs2812Pin, .ledCount = ledCount},
        .statusLed = PC13,
        .statusLedActiveLow = true,
    };
}

Stm32F411BoardConfig Stm32F411Boards::devEBox(const std::uint16_t ledCount) {
    return Stm32F411BoardConfig{
        .name = "DevEBox STM32F411CE",
        .sd = kExternalSdPins,
        .audio = kMax98357Pins,
        .input = kInputPins,
        .lights = Stm32F411LightPins{.data = kWs2812Pin, .ledCount = ledCount},
        .statusLed = std::nullopt,
        .statusLedActiveLow = true,
    };
}
