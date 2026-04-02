#ifndef BLASTER_FIRMWARE_PCINPUT_H
#define BLASTER_FIRMWARE_PCINPUT_H

#pragma once

#include "core/input/IInput.h"

class ITime;

class PCInput final : public IInput {
public:
    explicit PCInput(ITime* time);

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;
};


#endif // BLASTER_FIRMWARE_PCINPUT_H