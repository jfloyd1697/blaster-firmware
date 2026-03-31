#ifndef BLASTER_FIRMWARE_PCINPUT_H
#define BLASTER_FIRMWARE_PCINPUT_H

#pragma once

#include <chrono>

#include "IInput.h"

class PCInput final : public IInput {
public:
    PCInput() = default;

protected:
    bool readRawButton(ButtonID button) const override {
        switch (button) {
            case ButtonID::Trigger:
                return GetAsyncKeyState(VK_SPACE) & 0x8000;
            case ButtonID::NextWeapon:
                return GetAsyncKeyState(VK_RIGHT) & 0x8000;
            case ButtonID::PreviousWeapon:
                return GetAsyncKeyState(VK_LEFT) & 0x8000;
            case ButtonID::Quit:
                return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
            default:
                return false;
        }
    }

    std::uint32_t nowMs() const override {
        using namespace std::chrono;
        const auto now = steady_clock::now().time_since_epoch();
        return static_cast<std::uint32_t>(duration_cast<milliseconds>(now).count());
    }
};

#endif // BLASTER_FIRMWARE_PCINPUT_H