//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_IINPUT_H
#define BLASTER_FIRMWARE_IINPUT_H

#pragma once

#include <unordered_map>

enum class ButtonID {
    Trigger,
    NextWeapon,
    PreviousWeapon,
    Quit,
    // Add more buttons here
};

class IInput {
public:
    virtual ~IInput() = default;

    bool isTriggerPressed() {
        return isButtonPressed(ButtonID::Trigger);
    }

    bool isNextPressed() {
        return isButtonPressed(ButtonID::NextWeapon);
    }

    bool isPrevPressed() {
        return isButtonPressed(ButtonID::PreviousWeapon);
    }

    bool isQuitPressed() {
        return isButtonPressed(ButtonID::Quit);
    }

    protected:
        // Concrete implementations must define how to read raw button states
        virtual bool readRawButton(ButtonID button) = 0;

        bool isButtonPressed(const ButtonID button) {
            const bool current = readRawButton(button); // platform-specific input

            const auto now = std::chrono::steady_clock::now();

            // Check if the button state changed
            if (auto &[lastState, lastChange] = buttonStates[button]; current != lastState) {
                if (const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChange).count(); dt >= debounceMs) {
                    lastState = current;
                    lastChange = now;
                    if (current) return true; // rising edge
                }
            }
            return false;
        }

    private:
        struct ButtonState {
            bool lastState = false;
            std::chrono::steady_clock::time_point lastChange = std::chrono::steady_clock::now();
        };

        std::unordered_map<ButtonID, ButtonState> buttonStates;
        static constexpr int debounceMs = 50; // adjust as needed
};

#endif //BLASTER_FIRMWARE_IINPUT_H