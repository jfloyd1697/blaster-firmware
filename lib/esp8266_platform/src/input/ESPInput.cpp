#include "platform/esp8266/input/ESPInput.h"

#include <Arduino.h>

#include "ITime.h"

ESPInput::ESPInput(const PinConfig& pins, ITime* time)
    : m_pins(pins), m_time(time) {
}

void ESPInput::begin() {
    if (m_pins.trigger >= 0) {
        pinMode(m_pins.trigger, INPUT_PULLDOWN_16);
    }

    if (m_pins.quit >= 0) {
        pinMode(m_pins.quit, INPUT_PULLUP);
    }
}

bool ESPInput::readRawButton(ButtonID button) const {
    switch (button) {
        case ButtonID::Trigger:
            return (m_pins.trigger >= 0) &&
                   (digitalRead(m_pins.trigger) == HIGH);

        case ButtonID::Quit:
            return (m_pins.quit >= 0) &&
                   (digitalRead(m_pins.quit) == LOW);

        case ButtonID::NextWeapon:
        case ButtonID::PreviousWeapon:
        case ButtonID::Reload: {
            if (m_pins.ladder < 0) {
                return false;
            }

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
    }

    return false;
}

std::uint32_t ESPInput::nowMs() const {
    if (m_time != nullptr) {
        return static_cast<std::uint32_t>(m_time->millis());
    }

    return ::millis();
}

int ESPInput::readLadderRaw() const {
    return analogRead(m_pins.ladder);
}