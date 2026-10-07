#ifndef BLASTER_FIRMWARE_ESPTIME_H
#define BLASTER_FIRMWARE_ESPTIME_H

#include <Arduino.h>

#include "core/time/ITime.h"

struct ESPTime : ITime {
    [[nodiscard]] unsigned long millis() const override {
        return ::millis();
    }
};


#endif //BLASTER_FIRMWARE_ESPTIME_H