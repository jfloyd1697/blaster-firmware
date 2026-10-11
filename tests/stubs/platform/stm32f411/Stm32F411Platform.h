#pragma once
#include "core/Platform.h"
struct Stm32F411PlatformFactory {
    static PlatformServices createConfiguredBoard();
};
