#ifndef BLASTER_FIRMWARE_PCINPUT_H
#define BLASTER_FIRMWARE_PCINPUT_H

#pragma once

#include <chrono>

#include "core/input/IInput.h"
#include "core/time/ITime.h"

class PCInput final : public IInput {
public:
    explicit PCInput(ITime* time);

protected:
    bool readRawButton(ButtonID button) const override;

    static bool isKeyDown(int vk);
};

#endif // BLASTER_FIRMWARE_PCINPUT_H