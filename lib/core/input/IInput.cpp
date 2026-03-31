#include "IInput.h"

IInput::IInput() = default;

IInput::ButtonState& IInput::stateFor(const ButtonID button) {
    return buttonStates_[toIndex(button)];
}

const IInput::ButtonState& IInput::stateFor(const ButtonID button) const {
    return buttonStates_[toIndex(button)];
}

const IInput::ButtonConfig& IInput::configFor(const ButtonID button) const {
    return buttonConfigs_[toIndex(button)];
}

void IInput::update() {
    const std::uint32_t now = nowMs();

    for (const auto button : allButtons_) {
        updateButton(button, now);
    }
}

void IInput::updateButton(const ButtonID button, const std::uint32_t now) {
    auto& state = stateFor(button);
    const auto& config = configFor(button);
    const bool rawPressed = readRawButton(button);

    state.pressedEvent = false;
    state.releasedEvent = false;
    state.shortPressedEvent = false;
    state.longPressedEvent = false;

    if (!state.initialized) {
        state.initialized = true;
        state.stablePressed = rawPressed;
        state.lastRawPressed = rawPressed;
        state.lastChangeMs = now;
        return;
    }

    if (rawPressed != state.lastRawPressed) {
        state.lastRawPressed = rawPressed;
        state.lastChangeMs = now;
    }

    if ((now - state.lastChangeMs) >= config.debounceMs) {
        if (state.stablePressed != rawPressed) {
            state.stablePressed = rawPressed;

            if (state.stablePressed) {
                state.pressedEvent = true;
                state.pressStartMs = now;
                state.longReported = false;
            } else {
                state.releasedEvent = true;

                if (config.enableShortPress && !state.longReported) {
                    state.shortPressedEvent = true;
                }
            }
        }
    }

    if (config.enableLongPress &&
        state.stablePressed &&
        !state.longReported &&
        (now - state.pressStartMs >= config.longPressMs)) {
        state.longReported = true;
        state.longPressedEvent = true;
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

bool IInput::wasReloadPressed() const {
    return stateFor(ButtonID::Reload).shortPressedEvent;
}

bool IInput::wasQuitPressed() const {
    return stateFor(ButtonID::Quit).pressedEvent;
}