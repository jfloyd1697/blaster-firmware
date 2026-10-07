#ifndef BLASTER_FIRMWARE_ITIME_H
#define BLASTER_FIRMWARE_ITIME_H

#pragma once

class ITime {
public:
    virtual ~ITime() = default;
    [[nodiscard]] virtual unsigned long millis() const = 0;
};

#endif // BLASTER_FIRMWARE_ITIME_H
