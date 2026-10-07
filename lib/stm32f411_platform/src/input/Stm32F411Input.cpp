#include "platform/stm32f411/input/Stm32F411Input.h"

#include <Arduino.h>

namespace {
constexpr int kNextUpper = 400;
constexpr int kPreviousUpper = 800;
constexpr int kReloadUpper = 3200;
}

Stm32F411Input::Stm32F411Input(const Stm32F411InputPins& pins, ITime* time)
    : IInput(time),
      m_pins(pins) {
}

void Stm32F411Input::begin() const {
    pinMode(m_pins.trigger, INPUT_PULLUP);
    pinMode(m_pins.ladder, INPUT_ANALOG);
    analogReadResolution(12);
}

bool Stm32F411Input::readRawButton(const ButtonID button) const {
    if (button == ButtonID::Trigger) {
        return digitalRead(m_pins.trigger) == LOW;
    }

    if (button == ButtonID::Quit) {
        return false;
    }

    const int raw = readLadderRaw();

    if (button == ButtonID::NextWeapon) {
        return raw < kNextUpper;
    }

    if (button == ButtonID::PreviousWeapon) {
        return raw >= kNextUpper && raw < kPreviousUpper;
    }

    if (button == ButtonID::Reload) {
        return raw >= kPreviousUpper && raw < kReloadUpper;
    }

    return false;
}

int Stm32F411Input::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}
