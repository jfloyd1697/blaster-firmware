#include "platform/teensy/input/TeensyInput.h"

#include <Arduino.h>

#include "core/time/ITime.h"

TeensyInput::TeensyInput(PinConfig pins, ITime* time, LadderConfig ladder)
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

    const int raw = readLadderRaw();

    if (button == ButtonID::NextWeapon) {
        return raw < m_ladder.nextMaximum;
    }

    if (button == ButtonID::PreviousWeapon) {
        return raw >= m_ladder.nextMaximum && raw < m_ladder.previousMaximum;
    }

    if (button == ButtonID::Reload) {
        return raw >= m_ladder.previousMaximum && raw < m_ladder.reloadMaximum;
    }

    return false;
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
