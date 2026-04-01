#pragma once

#include "core/input/IInput.h"
#include "core/time/ITime.h"


class ESPInput : public IInput {
public:
    struct PinConfig {
        int trigger = -1;
        int ladder = -1;
        int quit = -1;
    };

    ESPInput(const PinConfig &pins, ITime *time);

    void begin();

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;

private:
    int readLadderRaw() const;

    PinConfig m_pins;
};
