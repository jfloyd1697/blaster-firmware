#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "core/time/ITime.h"

enum class ButtonID : std::size_t {
    Trigger = 0,
    NextWeapon,
    PreviousWeapon,
    Reload,
    Quit,
};

class IInput {
public:
    explicit IInput(ITime *time);

    virtual ~IInput() = default;

    void update();

    [[nodiscard]] bool isTriggerPressed() const;

    [[nodiscard]] bool wasTriggerPressed() const;

    [[nodiscard]] bool wasTriggerReleased() const;

    [[nodiscard]] bool wasNextShortPressed() const;

    [[nodiscard]] bool wasNextLongPressed() const;

    [[nodiscard]] bool wasPrevShortPressed() const;

    [[nodiscard]] bool wasPrevLongPressed() const;

    [[nodiscard]] bool wasReloadPressed() const;

    [[nodiscard]] bool wasQuitPressed() const;

protected:
    [[nodiscard]] virtual bool readRawButton(ButtonID button) const = 0;

    [[nodiscard]] std::uint32_t nowMs() const;

private:
    struct ButtonConfig {
        bool enableShortPress = false;
        bool enableLongPress = false;
        std::uint32_t debounceMs = 20;
        std::uint32_t longPressMs = 500;
    };

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

    static constexpr std::size_t kButtonCount =
            static_cast<std::size_t>(ButtonID::Quit) + 1;

    static constexpr std::size_t toIndex(const ButtonID button) {
        return static_cast<std::size_t>(button);
    }

    static constexpr std::array<ButtonID, kButtonCount> allButtons_ = {
        ButtonID::Trigger,
        ButtonID::NextWeapon,
        ButtonID::PreviousWeapon,
        ButtonID::Reload,
        ButtonID::Quit
    };

    static constexpr std::array<ButtonConfig, kButtonCount> buttonConfigs_ = {
        {
            {false, false, 20, 500}, // Trigger
            {true, true, 20, 500}, // NextWeapon
            {true, true, 20, 500}, // PreviousWeapon
            {true, false, 20, 500}, // Reload
            {false, false, 20, 500} // Quit
        }
    };

    ButtonState &stateFor(ButtonID button);

    [[nodiscard]] const ButtonState &stateFor(ButtonID button) const;

    [[nodiscard]] const ButtonConfig &configFor(ButtonID button) const;

    void updateButton(ButtonID button, std::uint32_t now);

    std::array<ButtonState, kButtonCount> buttonStates_{};

    ITime *m_time = nullptr;
};
