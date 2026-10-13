#pragma once
#include <cstdint>
constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int INPUT_ANALOG = 3;
void pinMode(std::uint32_t pin, int mode);
int digitalRead(std::uint32_t pin);
int analogRead(std::uint32_t pin);
void analogReadResolution(int bits);
