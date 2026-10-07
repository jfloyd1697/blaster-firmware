#ifndef BLASTER_STM32F411_BOARD_CONFIG_H
#define BLASTER_STM32F411_BOARD_CONFIG_H

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <optional>

struct Stm32F411SpiPins {
    pin_size_t mosi;
    pin_size_t miso;
    pin_size_t sck;
    pin_size_t cs;
};

struct Stm32F411I2sPins {
    pin_size_t ws;
    pin_size_t bclk;
    pin_size_t data;
};

struct Stm32F411InputPins {
    pin_size_t trigger;
    pin_size_t ladder;
};

struct Stm32F411LightPins {
    pin_size_t data;
    std::uint16_t ledCount;
};

struct Stm32F411BoardConfig {
    const char* name;
    Stm32F411SpiPins sd;
    Stm32F411I2sPins audio;
    Stm32F411InputPins input;
    Stm32F411LightPins lights;
    std::optional<pin_size_t> statusLed;
    bool statusLedActiveLow;
};

struct Stm32F411Boards {
    [[nodiscard]] static Stm32F411BoardConfig blackPillV20(std::uint16_t ledCount);
    [[nodiscard]] static Stm32F411BoardConfig devEBox(std::uint16_t ledCount);
};

#endif // BLASTER_STM32F411_BOARD_CONFIG_H
