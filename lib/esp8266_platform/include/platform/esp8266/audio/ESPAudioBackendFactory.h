//
// Created by user on 3/31/2026.
//

#ifndef BLASTER_FIRMWARE_ESPAUDIOBACKENDFACTORY_H
#define BLASTER_FIRMWARE_ESPAUDIOBACKENDFACTORY_H

#pragma once

#include <memory>

class IAudioEngine;
class IDebug;
class ITime;

enum class ESPAudioBackendType {
    Basic,
    Overlap,
    PseudoOverlap
};

std::unique_ptr<IAudioEngine> createESPAudioEngine(
    ESPAudioBackendType type,
    IDebug* debug,
    ITime* time,
    std::size_t overlapVoices = 4,
    uint32_t pseudoOverlapMs = 30
);

#endif //BLASTER_FIRMWARE_ESPAUDIOBACKENDFACTORY_H
