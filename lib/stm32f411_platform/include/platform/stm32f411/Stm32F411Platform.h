#ifndef BLASTER_STM32F411_PLATFORM_H
#define BLASTER_STM32F411_PLATFORM_H

#pragma once

#include "core/Platform.h"
#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

struct Stm32F411PlatformFactory {
    [[nodiscard]] static PlatformServices create(const Stm32F411BoardConfig& board);
    [[nodiscard]] static PlatformServices createConfiguredBoard();
};

#endif // BLASTER_STM32F411_PLATFORM_H
