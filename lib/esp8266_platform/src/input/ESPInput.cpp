#include "platform/esp8266/input/ESPInput.h"

#include <Arduino.h>

#include "core/time/ITime.h"


ESPInput::ESPInput(const PinConfig &pins, ITime *time)
    : IInput(time),
      m_pins(pins) {
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
    }5

    const int raw = readLadderRaw();

    if (button == ButtonID::NextWeapon) {
        return raw < 100;
    }

    if (button == ButtonID::PreviousWeapon) {
        return raw >= 100 && raw < 200;
    }

    if (button == ButtonID::Reload) {
        return raw >= 200 && raw < 800;
    }

    return false;
}

int ESPInput::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}
