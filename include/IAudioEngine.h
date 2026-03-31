//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_IAUDIOENGINE_H
#define BLASTER_FIRMWARE_IAUDIOENGINE_H

#pragma once
#include <string>

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual void playSound(const std::string& file) = 0;
    virtual void stop() = 0;
};

#endif //BLASTER_FIRMWARE_IAUDIOENGINE_H