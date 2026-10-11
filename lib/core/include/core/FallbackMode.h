#ifndef BLASTER_FIRMWARE_FALLBACKMODE_H
#define BLASTER_FIRMWARE_FALLBACKMODE_H

#pragma once

#include "core/Platform.h"

class FallbackMode {
public:
    explicit FallbackMode(PlatformServices& services);

    void initialize();
    bool update();

private:
    PlatformServices& m_services;
};

#endif // BLASTER_FIRMWARE_FALLBACKMODE_H
