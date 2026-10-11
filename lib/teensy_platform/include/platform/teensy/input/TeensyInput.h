#ifndef BLASTER_TEENSY_INPUT_H
#define BLASTER_TEENSY_INPUT_H

#pragma once

#include "core/input/AnalogButtonLadder.h"
#include "core/input/IInput.h"

class TeensyInput : public IInput {
public:
    struct PinConfig {
        int trigger = -1;
        int ladder = -1;
        int quit = -1;
        bool triggerActiveLow = true;
        bool quitActiveLow = true;
    };

    TeensyInput(
        PinConfig pins,
        ITime* time,
        AnalogButtonLadderConfig ladder = {});

    void begin() const;
    [[nodiscard]] std::optional<int> rawLadderValue() const override;

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;

private:
    [[nodiscard]] int readLadderRaw() const;
    [[nodiscard]] static bool readDigitalButton(int pin, bool activeLow);

    PinConfig m_pins;
    AnalogButtonLadder m_ladder;
};

#endif // BLASTER_TEENSY_INPUT_H
