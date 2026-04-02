//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_SHOOTCONTEXT_H
#define BLASTER_FIRMWARE_SHOOTCONTEXT_H

#pragma once

#include <functional>

class ITime;
class IAudioEngine;
struct IDebug;
struct WeaponProfile;

struct ShootContext {
    ITime* time = nullptr;
    IAudioEngine* audio = nullptr;
    IDebug* debug = nullptr;

    const WeaponProfile* profile = nullptr;
    int* ammo = nullptr;

    std::function<void()> emitShot;
    std::function<void()> flashMuzzle;
};

#endif //BLASTER_FIRMWARE_SHOOTCONTEXT_H