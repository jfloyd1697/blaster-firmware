//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_ITIME_H
#define BLASTER_FIRMWARE_ITIME_H

#pragma once
#include <cstdint>

class ITime {
public:
    virtual ~ITime() = default;
    virtual uint64_t millis() const = 0;
};

#endif //BLASTER_FIRMWARE_ITIME_H