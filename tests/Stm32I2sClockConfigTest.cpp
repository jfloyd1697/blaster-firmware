#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <stm32f4xx_hal.h>

#include "core/debug/IDebug.h"
#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

StubSd g_stm32Sd;

struct TestDebug final : IDebug {
    std::vector<std::string> errors;
    void log(const std::string&) override {}
    void error(const std::string& value) override { errors.push_back(value); }
};

int main() {
    TestDebug debug;
    Stm32F411AudioBackend backend({PB12, PB13, PB15}, &debug);

    // PLLI2SM is a separate required divider on the STM32F411.
    // Derive it from the working system PLL source divider, never leave it zero.
    RCC->PLLCFGR = 25u;
    assert(backend.begin());
    assert(testConfiguredPllI2sM == 25);
    assert(testI2sSelectedClockSource == RCC_I2SCLKSOURCE_PLLI2S);
    assert(testI2sInitCount == 1);
    assert(debug.errors.empty());

    // An invalid system clock divider is rejected before HAL attempts I2S init.
    RCC->PLLCFGR = 0u;
    Stm32F411AudioBackend invalidBackend({PB12, PB13, PB15}, &debug);
    assert(!invalidBackend.begin());
    assert(testI2sInitCount == 1);
}
