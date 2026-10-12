#include <cassert>
#include <cstdint>

#include <stm32f4xx_hal.h>

#include "platform/stm32f411/diagnostics/I2sGpioLoopback.h"

int main() {
    using namespace stm32_i2s_loopback;
    assert(begin());
    assert((TIM2->CR1 & TIM_CR1_CEN) != 0u);
    assert((TIM2->CCMR1 & TIM_CCMR1_CC1S_0) != 0u);
    assert((TIM2->CCER & TIM_CCER_CC1E) != 0u);
    const std::uint32_t expectedTrigger = TIM_SMCR_TS_2 | TIM_SMCR_TS_0;
    const std::uint32_t expectedClockMode = TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0;
    assert((TIM2->SMCR & (expectedTrigger | expectedClockMode)) ==
           (expectedTrigger | expectedClockMode));
    TIM2->CNT = 11025u;
    assert(takeRisingEdges() == 11025u);
    assert(TIM2->CNT == 0u);
    assert(runReferencePulseTest() == 0u); // Stub does not simulate real jumper edges.
    TIM2->CNT = 123u;
    resetCounter();
    assert(TIM2->CNT == 0u);
    end();
    assert((TIM2->CR1 & TIM_CR1_CEN) == 0u);
    assert(takeRisingEdges() == 0u);
}
