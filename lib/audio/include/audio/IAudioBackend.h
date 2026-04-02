//
// Created by user on 3/31/2026.
//

#ifndef BLASTER_FIRMWARE_IAUDIOBACKEND_H
#define BLASTER_FIRMWARE_IAUDIOBACKEND_H

#pragma once

#include <string>

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual void playSound(const std::string& file, bool loop) = 0;
    virtual void stop() = 0;
};

#endif //BLASTER_FIRMWARE_IAUDIOBACKEND_H