#ifndef BLASTER_ESPINPUT_H
#define BLASTER_ESPINPUT_H

#pragma once

#include <Arduino.h>
#include <cstdint>

#include "IInput.h"

class ESPInput final : public IInput {
public:
    struct PinConfig {
        int trigger = -1;
        int nextWeapon = -1;
        int previousWeapon = -1;
        int quit = -1;
    };

    explicit ESPInput(const PinConfig& pins)
        : m_pins(pins) {
    }

    void begin() const {
        if (m_pins.trigger >= 0) {
            pinMo de(m_pins.trigger, INPUT_PULLUP);
        }
        if (m_pins.nextWeapon >= 0) {
            pinMode(m_pins.nextWeapon, INPUT_PULLUP);
        }
        if (m_pins.previousWeapon >= 0) {
            pinMode(m_pins.previousWeapon, INPUT_PULLUP);
        }
        if (m_pins.quit >= 0) {
            pinMode(m_pins.quit, INPUT_PULLUP);
        }
    }

protected:
    bool readRawButton(const ButtonID button) const override {
        switch (button) {
            case ButtonID::Trigger:
                return isPressed(m_pins.trigger);
            case ButtonID::NextWeapon:
                return isPressed(m_pins.nextWeapon);
            case ButtonID::PreviousWeapon:
                return isPressed(m_pins.previousWeapon);
            case ButtonID::Quit:
                return isPressed(m_pins.quit);
            default:
                return false;
        }
    }

    std::uint32_t nowMs() const override {
        return static_cast<std::uint32_t>(millis());
    }

private:
    static bool isPressed(const int pin) {
        if (pin < 0) {
            return false;
        }
        return digitalRead(pin) == LOW;
    }

    PinConfig m_pins;
};

#endif