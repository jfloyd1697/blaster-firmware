#include "platform/stm32f411/diagnostics/I2sGpioLoopback.h"

#include <Arduino.h>
#include <stm32f4xx_hal.h>

namespace stm32_i2s_loopback {
namespace {
bool running = false;
}

bool begin() {
    if (running) {
        return true;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // PA15 is TIM2_CH1 (AF1) on STM32F411CE. Use an internal pull-down so
    // an unconnected jumper does not look like an active clock.
    GPIO_InitTypeDef gpio{};
    gpio.Pin = GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &gpio);

    // TIM2 external-clock mode 1: TI1FP1 -> trigger -> rising-edge counter.
    // This is hardware counting; no interrupt is generated per I2S edge.
    TIM2->CR1 = 0u;
    TIM2->SMCR = 0u;
    TIM2->CCER = 0u;
    TIM2->CCMR1 = TIM_CCMR1_CC1S_0;
    TIM2->PSC = 0u;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->CNT = 0u;
    TIM2->CCER = TIM_CCER_CC1E;
    TIM2->SMCR = TIM_SMCR_TS_2 | TIM_SMCR_TS_0 |
                 TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_CEN;
    running = true;
    return true;
}

void resetCounter() {
    if (running) {
        TIM2->CNT = 0u;
    }
}

std::uint32_t takeRisingEdges() {
    if (!running) {
        return 0u;
    }
    const std::uint32_t edges = TIM2->CNT;
    TIM2->CNT = 0u;
    return edges;
}

std::uint32_t runReferencePulseTest() {
    if (!running) {
        return 0u;
    }

    // PB8 is free in the STM32F411 board configuration (PB6 drives WS2812).
    // Use ordinary GPIO, not a timer or DMA, so this tests the PA15/TIM2
    // capture path without relying on I2S configuration.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio{};
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    delayMicroseconds(1000);
    resetCounter();
    for (std::uint32_t index = 0; index < 250u; ++index) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        delayMicroseconds(500);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        delayMicroseconds(500);
    }
    const std::uint32_t edges = takeRisingEdges();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    return edges;
}

void end() {
    if (!running) {
        return;
    }
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->SMCR = 0u;
    TIM2->CCER = 0u;
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);
    __HAL_RCC_TIM2_CLK_DISABLE();
    running = false;
}

}  // namespace stm32_i2s_loopback
