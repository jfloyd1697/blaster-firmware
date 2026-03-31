#pragma once

#include <cstdint>

#include "input/IInput.h"

class ITime;

class ESPInput : public IInput {
public:
    struct PinConfig {
        int trigger = -1;
        int ladder  = -1;
        int quit    = -1;
    };

    ESPInput(const PinConfig& pins, ITime* time);

    void begin();

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;
    [[nodiscard]] std::uint32_t nowMs() const override;

private:
    int readLadderRaw() const;

private:
    PinConfig m_pins;
    ITime* m_time = nullptr;
};