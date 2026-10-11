#include "platform/stm32f411/input/Stm32F411Input.h"

#include <Arduino.h>

Stm32F411Input::Stm32F411Input(
    const Stm32F411InputPins& pins,
    ITime* time)
    : IInput(time),
      m_pins(pins),
      m_ladder() {
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
    return m_ladder.isPressed(button, readLadderRaw());
}

int Stm32F411Input::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}

std::optional<int> Stm32F411Input::rawLadderValue() const {
    return readLadderRaw();
}
