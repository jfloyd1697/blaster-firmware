#include "core/input/AnalogButtonLadder.h"

#include <utility>

AnalogButtonLadder::AnalogButtonLadder(AnalogButtonLadderConfig config)
    : m_config(std::move(config)) {
}

std::optional<ButtonID> AnalogButtonLadder::buttonForValue(const int value) const {
    if (value < m_config.nextMaximum) {
        return ButtonID::NextWeapon;
    }
    if (value < m_config.previousMaximum) {
        return ButtonID::PreviousWeapon;
    }
    if (value < m_config.reloadMaximum) {
        return ButtonID::Reload;
    }
    return std::nullopt;
}

bool AnalogButtonLadder::isPressed(const ButtonID button, const int value) const {
    const auto active = buttonForValue(value);
    return active.has_value() && *active == button;
}

const AnalogButtonLadderConfig& AnalogButtonLadder::config() const {
    return m_config;
}
