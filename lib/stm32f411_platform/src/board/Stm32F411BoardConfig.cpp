#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

namespace {
constexpr Stm32F411SpiPins kExternalSdPins{
    .mosi = PA7,
    .miso = PA6,
    .sck = PA5,
    .cs = PA4,
};

constexpr Stm32F411I2sPins kMax98357Pins{
    .ws = PB12,
    .bclk = PB13,
    .data = PB15,
};

constexpr Stm32F411InputPins kInputPins{
    .trigger = PA0,
    .ladder = PA1,
};

constexpr std::uint32_t kWs2812Pin = PB6;
constexpr Stm32F411OledPins kOledPins{.scl = PB8, .sda = PB9, .address = 0x3C};
}

Stm32F411BoardConfig Stm32F411Boards::blackPillV20(const std::uint16_t ledCount) {
    return Stm32F411BoardConfig{
        .name = "STM32F4x1Cx v2.0+",
        .sd = kExternalSdPins,
        .audio = kMax98357Pins,
        .input = kInputPins,
        .lights = Stm32F411LightPins{.data = kWs2812Pin, .ledCount = ledCount},
        .oled = kOledPins,
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
        .oled = kOledPins,
        .statusLed = std::nullopt,
        .statusLedActiveLow = true,
    };
}
