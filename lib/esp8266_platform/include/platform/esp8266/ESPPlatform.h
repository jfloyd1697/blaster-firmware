#ifndef BLASTER_ESPPLATFORM_H
#define BLASTER_ESPPLATFORM_H

#include "core/Platform.h"

struct ESPPlatformFactory {
    static PlatformServices create();
};

#endif