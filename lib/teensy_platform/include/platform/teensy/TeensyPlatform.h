#ifndef BLASTER_TEENSY_PLATFORM_H
#define BLASTER_TEENSY_PLATFORM_H

#pragma once

#include "core/Platform.h"

struct TeensyPlatformFactory {
    static PlatformServices create();
};

#endif // BLASTER_TEENSY_PLATFORM_H
