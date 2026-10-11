#ifndef BLASTER_STM32F411_LIGHT_BACKEND_H
#define BLASTER_STM32F411_LIGHT_BACKEND_H

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/lights/ILightBackend.h"
#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

struct IDebug;

class Stm32F411LightBackend : public ILightBackend {
public:
    explicit Stm32F411LightBackend(const Stm32F411LightPins& pins, IDebug* debug = nullptr);
    ~Stm32F411LightBackend() override;

    bool begin() override;
    [[nodiscard]] std::size_t ledCount() const override;
    void show(const std::vector<LightColor>& pixels) override;

    void onDmaComplete();

private:
    bool initializeTimerDma();
    void waitForTransfer();

    Stm32F411LightPins m_pins;
    IDebug* m_debug = nullptr;
    std::uint16_t m_zeroDutyTicks = 0;
    std::uint16_t m_oneDutyTicks = 0;
    bool m_initialized = false;
    bool m_reportedTransferError = false;
    std::vector<std::uint16_t> m_pwmData;
    bool m_transferActive = false;
};

#endif // BLASTER_STM32F411_LIGHT_BACKEND_H
