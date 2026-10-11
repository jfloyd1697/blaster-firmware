#pragma once

#include "core/input/AnalogButtonLadder.h"
#include "core/input/IInput.h"

class ESPInput : public IInput {
public:
    struct PinConfig {
        int trigger = -1;
        int ladder = -1;
        int quit = -1;
    };

    ESPInput(PinConfig pins, ITime* time, AnalogButtonLadderConfig ladder = {});

    void begin() const;
    [[nodiscard]] std::optional<int> rawLadderValue() const override;

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;

private:
    [[nodiscard]] int readLadderRaw() const;

    PinConfig m_pins;
    AnalogButtonLadder m_ladder;
};
