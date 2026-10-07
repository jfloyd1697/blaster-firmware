#ifndef BLASTER_FIRMWARE_SHOOTCONTEXT_H
#define BLASTER_FIRMWARE_SHOOTCONTEXT_H

#pragma once

#include <functional>
#include <string>
#include <vector>

class ITime;
class IAudioEngine;
struct IDebug;

namespace weapon_behavior {
struct LightPatternDef;
}

struct ShootContext {
    ITime* time = nullptr;
    IAudioEngine* audio = nullptr;
    IDebug* debug = nullptr;

    int* ammo = nullptr;

    std::function<void()> emitShot;
    std::function<void()> flashMuzzle;

    std::function<void(const weapon_behavior::LightPatternDef&)> setLight;
    std::function<void(const weapon_behavior::LightPatternDef&)> flashLight;

    std::function<void(const std::string&, bool, bool)> playSound;
    std::function<void(const std::vector<std::string>&, bool, bool)> playRandomSound;
    std::function<void()> stopSound;

    std::function<void(const std::string&)> emitBehaviorEvent;
};

#endif // BLASTER_FIRMWARE_SHOOTCONTEXT_H
