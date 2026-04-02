#ifndef BLASTER_FIRMWARE_SHOOTCONTEXT_H
#define BLASTER_FIRMWARE_SHOOTCONTEXT_H

#pragma once

#include <functional>
#include <string>
#include <vector>

class ITime;
class IAudioEngine;
struct IDebug;
struct WeaponProfile;
namespace weapon_behavior {
    struct LightPatternDef;
}

using namespace weapon_behavior;

struct ShootContext {
    ITime* time = nullptr;
    IAudioEngine* audio = nullptr;
    IDebug* debug = nullptr;

    const WeaponProfile* profile = nullptr;
    int* ammo = nullptr;

    std::function<void()> emitShot;
    std::function<void()> flashMuzzle;

    std::function<void(const LightPatternDef&)> setLight;
    std::function<void(const LightPatternDef&)> flashLight;

    std::function<void(const std::string&, bool)> playSound;
    std::function<void(const std::vector<std::string>&, bool)> playRandomSound;
    std::function<void()> stopSound;

    std::function<void(const std::string&)> emitBehaviorEvent;
};

#endif // BLASTER_FIRMWARE_SHOOTCONTEXT_H