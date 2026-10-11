#include "platform/esp8266/input/ESPInput.h"

#include <Arduino.h>

#include "core/time/ITime.h"

ESPInput::ESPInput(PinConfig pins, ITime* time, const AnalogButtonLadderConfig ladder)
    : IInput(time),
      m_pins(pins),
      m_ladder(ladder) {
}

void ESPInput::begin() const {
    if (m_pins.trigger >= 0) {
        pinMode(m_pins.trigger, INPUT_PULLDOWN_16);
    }
    if (m_pins.quit >= 0) {
        pinMode(m_pins.quit, INPUT_PULLUP);
    }
}

bool ESPInput::readRawButton(const ButtonID button) const {
    if (button == ButtonID::Trigger) {
        return (m_pins.trigger >= 0) && (digitalRead(m_pins.trigger) == HIGH);
    }
    if (button == ButtonID::Quit) {
        return (m_pins.quit >= 0) && (digitalRead(m_pins.quit) == LOW);
    }
    if (m_pins.ladder < 0) {
        return false;
    }
    return m_ladder.isPressed(button, readLadderRaw());
}

int ESPInput::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}

std::optional<int> ESPInput::rawLadderValue() const {
    if (m_pins.ladder < 0) {
        return std::nullopt;
    }
    return readLadderRaw();
}
