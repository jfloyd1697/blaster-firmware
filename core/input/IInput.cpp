#include "../../include/IInput.h"

IInput::IInput() {
    buttonStates_.emplace(ButtonID::Trigger, ButtonState{});
    buttonStates_.emplace(ButtonID::NextWeapon, ButtonState{});
    buttonStates_.emplace(ButtonID::PreviousWeapon, ButtonState{});
    buttonStates_.emplace(ButtonID::Quit, ButtonState{});
}

void IInput::update() {
    const std::uint32_t t = nowMs();

    updateButton(ButtonID::Trigger, t);
    updateButton(ButtonID::NextWeapon, t);
    updateButton(ButtonID::PreviousWeapon, t);
    updateButton(ButtonID::Quit, t);
}

IInput::ButtonState& IInput::stateFor(const ButtonID button) {
    return buttonStates_[button];
}

const IInput::ButtonState& IInput::stateFor(const ButtonID button) const {
    return buttonStates_.at(button);
}

void IInput::updateButton(const ButtonID button, const std::uint32_t t) {
    auto& s = stateFor(button);
    const bool rawPressed = readRawButton(button);

    s.pressedEvent = false;
    s.releasedEvent = false;
    s.shortPressedEvent = false;
    s.longPressedEvent = false;

    if (!s.initialized) {
        s.initialized = true;
        s.stablePressed = rawPressed;
        s.lastRawPressed = rawPressed;
        s.lastChangeMs = t;
        return;
    }

    if (rawPressed != s.lastRawPressed) {
        s.lastRawPressed = rawPressed;
        s.lastChangeMs = t;
    }

    if ((t - s.lastChangeMs) >= debounceMs_) {
        if (s.stablePressed != rawPressed) {
            s.stablePressed = rawPressed;

            if (s.stablePressed) {
                s.pressedEvent = true;
                s.pressStartMs = t;
                s.longReported = false;
            } else {
                s.releasedEvent = true;
                if (!s.longReported) {
                    s.shortPressedEvent = true;
                }
            }
        }
    }

    if (s.stablePressed && !s.longReported && (t - s.pressStartMs >= longPressMs_)) {
        s.longReported = true;
        s.longPressedEvent = true;
    }
}

bool IInput::isTriggerPressed() const {
    return stateFor(ButtonID::Trigger).stablePressed;
}

bool IInput::wasTriggerPressed() const {
    return stateFor(ButtonID::Trigger).pressedEvent;
}

bool IInput::wasTriggerReleased() const {
    return stateFor(ButtonID::Trigger).releasedEvent;
}

bool IInput::wasNextShortPressed() const {
    return stateFor(ButtonID::NextWeapon).shortPressedEvent;
}

bool IInput::wasNextLongPressed() const {
    return stateFor(ButtonID::NextWeapon).longPressedEvent;
}

bool IInput::wasPrevShortPressed() const {
    return stateFor(ButtonID::PreviousWeapon).shortPressedEvent;
}

bool IInput::wasPrevLongPressed() const {
    return stateFor(ButtonID::PreviousWeapon).longPressedEvent;
}

bool IInput::wasQuitPressed() const {
    return stateFor(ButtonID::Quit).pressedEvent;
}