#ifndef BLASTER_STM32F411_INPUT_H
#define BLASTER_STM32F411_INPUT_H

#pragma once

#include "core/input/IInput.h"
#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

class Stm32F411Input : public IInput {
public:
    Stm32F411Input(const Stm32F411InputPins& pins, ITime* time);

    void begin() const;

protected:
    [[nodiscard]] bool readRawButton(ButtonID button) const override;

private:
    [[nodiscard]] int readLadderRaw() const;

    Stm32F411InputPins m_pins;
};

#endif // BLASTER_STM32F411_INPUT_H
