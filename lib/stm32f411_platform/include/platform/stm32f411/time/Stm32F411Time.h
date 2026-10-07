#ifndef BLASTER_STM32F411_TIME_H
#define BLASTER_STM32F411_TIME_H

#pragma once

#include <Arduino.h>

#include "core/time/ITime.h"

struct Stm32F411Time : ITime {
    [[nodiscard]] unsigned long millis() const override {
        return ::millis();
    }
};

#endif // BLASTER_STM32F411_TIME_H
