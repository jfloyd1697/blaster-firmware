#ifndef BLASTER_FIRMWARE_IINPUT_H
#define BLASTER_FIRMWARE_IINPUT_H

#pragma once

#include <cstdint>
#include <unordered_map>

enum class ButtonID {
    Trigger,
    NextWeapon,
    PreviousWeapon,
    Quit,
};

class IInput {
public:
    IInput();
    virtual ~IInput() = default;

    void update();

    bool isTriggerPressed() const;

    bool wasTriggerPressed() const;
    bool wasTriggerReleased() const;

    bool wasNextShortPressed() const;
    bool wasNextLongPressed() const;

    bool wasPrevShortPressed() const;
    bool wasPrevLongPressed() const;

    bool wasQuitPressed() const;

protected:
    virtual bool readRawButton(ButtonID button) const = 0;
    virtual std::uint32_t nowMs() const = 0;

private:
    struct ButtonState {
        bool stablePressed = false;
        bool lastRawPressed = false;

        bool pressedEvent = false;
        bool releasedEvent = false;
        bool shortPressedEvent = false;
        bool longPressedEvent = false;

        bool longReported = false;

        std::uint32_t lastChangeMs = 0;
        std::uint32_t pressStartMs = 0;
        bool initialized = false;
    };

    ButtonState& stateFor(ButtonID button);
    const ButtonState& stateFor(ButtonID button) const;

    void updateButton(ButtonID button, std::uint32_t t);

    std::unordered_map<ButtonID, ButtonState> buttonStates_;

    static constexpr std::uint32_t debounceMs_ = 10;
    static constexpr std::uint32_t longPressMs_ = 500;
};

#endif