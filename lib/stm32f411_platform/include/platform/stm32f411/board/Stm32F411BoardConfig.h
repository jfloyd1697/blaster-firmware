#ifndef BLASTER_STM32F411_BOARD_CONFIG_H
#define BLASTER_STM32F411_BOARD_CONFIG_H

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <optional>


struct Stm32F411SpiPins {
    std::uint32_t mosi;
    std::uint32_t miso;
    std::uint32_t sck;
    std::uint32_t cs;
};

struct Stm32F411I2sPins {
    std::uint32_t ws;
    std::uint32_t bclk;
    std::uint32_t data;
};

struct Stm32F411InputPins {
    std::uint32_t trigger;
    std::uint32_t ladder;
};

struct Stm32F411LightPins {
    std::uint32_t data;
    std::uint16_t ledCount;
};

struct Stm32F411BoardConfig {
    const char* name;
    Stm32F411SpiPins sd;
    Stm32F411I2sPins audio;
    Stm32F411InputPins input;
    Stm32F411LightPins lights;
    std::optional<std::uint32_t> statusLed;
    bool statusLedActiveLow;
};

struct Stm32F411Boards {
    [[nodiscard]] static Stm32F411BoardConfig blackPillV20(std::uint16_t ledCount);
    [[nodiscard]] static Stm32F411BoardConfig devEBox(std::uint16_t ledCount);
};

#endif // BLASTER_STM32F411_BOARD_CONFIG_H
