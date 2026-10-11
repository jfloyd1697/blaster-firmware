#ifndef BLASTER_FIRMWARE_ANALOGBUTTONLADDER_H
#define BLASTER_FIRMWARE_ANALOGBUTTONLADDER_H

#pragma once

#include <optional>

#include "core/input/IInput.h"

struct AnalogButtonLadderConfig {
    int nextMaximum = 245;
    int previousMaximum = 850;
    int reloadMaximum = 2700;
};

class AnalogButtonLadder {
public:
    explicit AnalogButtonLadder(AnalogButtonLadderConfig config = {});

    [[nodiscard]] std::optional<ButtonID> buttonForValue(int value) const;
    [[nodiscard]] bool isPressed(ButtonID button, int value) const;
    [[nodiscard]] const AnalogButtonLadderConfig& config() const;

private:
    AnalogButtonLadderConfig m_config;
};

#endif // BLASTER_FIRMWARE_ANALOGBUTTONLADDER_H
