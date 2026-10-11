#ifndef BLASTER_FIRMWARE_SHOOTCONTEXT_H
#define BLASTER_FIRMWARE_SHOOTCONTEXT_H

#pragma once

#include <functional>
#include <string>
#include <vector>

class ITime;
class IAudioEngine;
struct IDebug;

struct ShootContext {
    ITime* time = nullptr;
    IAudioEngine* audio = nullptr;
    IDebug* debug = nullptr;

    std::function<void(const std::string&, bool, const std::string&)> playAudio;
    std::function<void()> stopAudio;
    std::function<void(const std::string&, bool)> playLight;
    std::function<void()> stopLight;
    std::function<void(const std::string&)> log;
};

#endif // BLASTER_FIRMWARE_SHOOTCONTEXT_H
