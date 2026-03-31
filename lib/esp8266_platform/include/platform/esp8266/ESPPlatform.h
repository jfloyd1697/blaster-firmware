#ifndef BLASTER_ESPPLATFORM_H
#define BLASTER_ESPPLATFORM_H

#include "Platform.h"

struct ESPPlatformFactory {
    static PlatformServices create();
};

#endif