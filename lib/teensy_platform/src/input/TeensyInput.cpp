#include "platform/teensy/input/TeensyInput.h"

#include <Arduino.h>

#include "core/time/ITime.h"

TeensyInput::TeensyInput(
    PinConfig pins,
    ITime* time,
    const AnalogButtonLadderConfig ladder)
    : IInput(time),
      m_pins(pins),
      m_ladder(ladder) {
}

void TeensyInput::begin() const {
    if (m_pins.trigger >= 0) {
        pinMode(m_pins.trigger, m_pins.triggerActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);
    }
    if (m_pins.ladder >= 0) {
        pinMode(m_pins.ladder, INPUT);
    }
    if (m_pins.quit >= 0) {
        pinMode(m_pins.quit, m_pins.quitActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);
    }
}

bool TeensyInput::readRawButton(const ButtonID button) const {
    if (button == ButtonID::Trigger) {
        return readDigitalButton(m_pins.trigger, m_pins.triggerActiveLow);
    }
    if (button == ButtonID::Quit) {
        return readDigitalButton(m_pins.quit, m_pins.quitActiveLow);
    }
    if (m_pins.ladder < 0) {
        return false;
    }
    return m_ladder.isPressed(button, readLadderRaw());
}

int TeensyInput::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}

bool TeensyInput::readDigitalButton(const int pin, const bool activeLow) {
    if (pin < 0) {
        return false;
    }
    const bool high = digitalRead(pin) == HIGH;
    return activeLow ? !high : high;
}

std::optional<int> TeensyInput::rawLadderValue() const {
    if (m_pins.ladder < 0) {
        return std::nullopt;
    }
    return readLadderRaw();
}
