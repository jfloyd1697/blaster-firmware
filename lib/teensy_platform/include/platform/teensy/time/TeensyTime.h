#ifndef BLASTER_TEENSY_TIME_H
#define BLASTER_TEENSY_TIME_H

#pragma once

#include <Arduino.h>

#include "core/time/ITime.h"

class TeensyTime : public ITime {
public:
    unsigned long millis() const override {
        return ::millis();
    }
};

#endif // BLASTER_TEENSY_TIME_H
