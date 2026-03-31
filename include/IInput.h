#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class ButtonID : std::size_t {
    Trigger = 0,
    NextWeapon,
    PreviousWeapon,
    Reload,
    Quit,
    Count
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

    bool wasReloadPressed() const;
    bool wasQuitPressed() const;

protected:
    virtual bool readRawButton(ButtonID button) const = 0;
    virtual std::uint32_t nowMs() const = 0;

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
        static_cast<std::size_t>(ButtonID::Count);

    static constexpr std::size_t toIndex(const ButtonID button) {
        return static_cast<std::size_t>(button);
    }

    inline static constexpr std::array<ButtonID, kButtonCount> allButtons_ = {
        ButtonID::Trigger,
        ButtonID::NextWeapon,
        ButtonID::PreviousWeapon,
        ButtonID::Reload,
        ButtonID::Quit
    };

    inline static constexpr std::array<ButtonConfig, kButtonCount> buttonConfigs_ = {{
        {false, false, 20, 500}, // Trigger
        {true,  true,  20, 500}, // NextWeapon
        {true,  true,  20, 500}, // PreviousWeapon
        {true,  false, 20, 500}, // Reload
        {false, false, 20, 500}  // Quit
    }};

    ButtonState& stateFor(ButtonID button);
    const ButtonState& stateFor(ButtonID button) const;

    const ButtonConfig& configFor(ButtonID button) const;

    void updateButton(ButtonID button, std::uint32_t now);

    std::array<ButtonState, kButtonCount> buttonStates_{};
};