#ifndef BLASTER_FIRMWARE_PLATFORM_H
#define BLASTER_FIRMWARE_PLATFORM_H

#pragma once

#include <memory>

class IAudioEngine;
struct IDebug;
class IInput;
class ILights;
class ITime;
class ITextResourceLoader;

struct PlatformServices {
    std::shared_ptr<IAudioEngine> audio;
    std::shared_ptr<IDebug> debug;
    std::shared_ptr<IInput> input;
    std::shared_ptr<ILights> lights;
    std::shared_ptr<ITime> time;
    std::shared_ptr<ITextResourceLoader> textLoader;
};

#endif // BLASTER_FIRMWARE_PLATFORM_H
