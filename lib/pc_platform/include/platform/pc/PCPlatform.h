#ifndef BLASTER_FIRMWARE_PCPLATFORM_H
#define BLASTER_FIRMWARE_PCPLATFORM_H

#pragma once

#include "core/Platform.h"

struct PCPlatformFactory {
    static PlatformServices create();
};

#endif