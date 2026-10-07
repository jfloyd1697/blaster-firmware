#ifndef BLASTER_FIRMWARE_IDEBUG_H
#define BLASTER_FIRMWARE_IDEBUG_H

#pragma once

#include <string>

struct IDebug {
    virtual ~IDebug() = default;
    virtual void log(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

#endif // BLASTER_FIRMWARE_IDEBUG_H
