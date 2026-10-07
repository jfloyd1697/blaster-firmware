#ifndef BLASTER_TEENSY_INPUT_H
#define BLASTER_TEENSY_INPUT_H

#pragma once

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

    struct LadderConfig {
        int nextMaximum = 100;
        int previousMaximum = 200;
        int reloadMaximum = 800;
    };

    TeensyInput(PinConfig pins, ITime* time, LadderConfig ladder = {});

    void begin() const;

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;

private:
    [[nodiscard]] int readLadderRaw() const;
    [[nodiscard]] static bool readDigitalButton(int pin, bool activeLow);

    PinConfig m_pins;
    LadderConfig m_ladder;
};

#endif // BLASTER_TEENSY_INPUT_H
