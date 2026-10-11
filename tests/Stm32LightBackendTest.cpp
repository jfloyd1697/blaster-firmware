#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <stm32f4xx_hal.h>

#include "core/debug/IDebug.h"
#include "platform/stm32f411/lights/Stm32F411LightBackend.h"

struct TestDebug final : IDebug {
    std::vector<std::string> logs;
    std::vector<std::string> errors;
    void log(const std::string& text) override { logs.push_back(text); }
    void error(const std::string& text) override { errors.push_back(text); }
};

unsigned long millis() {
    static unsigned long now = 0;
    return now++;
}

extern "C" void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* handle);

int main() {
    TestDebug debug;
    // PCLK1=48MHz, timer=96MHz. The old driver rejected this valid clock.
    testRcc.CFGR = 4u << 10u;
    testPclk1Hz = 48000000u;
    {
        Stm32F411LightBackend lights({PB6, 1}, &debug);
        assert(lights.begin());
        assert(testTimPwmInitCount == 1);
        lights.show({LightColor{255, 0, 0}});
        assert(testTimPwmStartCount == 1);
        assert(testTimPwmLength == 24 + 64);
        const auto* pwm = reinterpret_cast<const std::uint16_t*>(testTimPwmData);
        // Red is encoded as GRB: 8 off bits, then 8 on bits, then 8 off bits.
        for (int i = 0; i < 8; ++i) assert(pwm[i] == 38);
        for (int i = 8; i < 16; ++i) assert(pwm[i] == 77);
        for (int i = 16; i < 24; ++i) assert(pwm[i] == 38);
        assert(pwm[24] == 0);
        TIM_HandleTypeDef complete;
        complete.Instance = TIM4;
        HAL_TIM_PWM_PulseFinishedCallback(&complete);
        assert(testTimPwmStopCount == 1);
        lights.show({LightColor{0, 255, 0}});
        assert(testTimPwmStartCount == 2);
    }
    assert(debug.errors.empty());
    // APB1 divisor = 1: TIM4 runs at PCLK1, not 2 x PCLK1.
    testRcc.CFGR = 0u;
    testPclk1Hz = 100000000u;
    Stm32F411LightBackend lights({PB6, 1}, &debug);
    assert(lights.begin());
    lights.show({LightColor{0, 0, 255}});
    const auto* pwm = reinterpret_cast<const std::uint16_t*>(testTimPwmData);
    assert(pwm[0] == 40);
    assert(pwm[16] == 80);
    assert(debug.errors.empty());
}
