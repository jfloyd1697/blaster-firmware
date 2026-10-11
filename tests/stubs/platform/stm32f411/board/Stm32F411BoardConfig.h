#pragma once
#include <cstdint>
constexpr std::uint32_t PB6 = 6;
constexpr std::uint32_t PB12 = 12;
constexpr std::uint32_t PB13 = 13;
constexpr std::uint32_t PB15 = 15;
struct Stm32F411I2sPins {
    std::uint32_t ws=PB12, bclk=PB13, data=PB15;
};

struct Stm32F411LightPins {
    std::uint32_t data = PB6;
    std::uint16_t ledCount = 1;
};
