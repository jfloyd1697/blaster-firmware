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
    std::vector<std::string> logs;
    std::vector<std::string> errors;
    void log(const std::string& text) override { logs.push_back(text); }
    void error(const std::string& text) override { errors.push_back(text); }
};

int main() {
    TestDebug debug;
    Stm32F411AudioBackend backend({PB12, PB13, PB15}, &debug);

    // Simulate the real problem: HAL fails at service startup. A later
    // diagnostic request must reinitialize the *whole* peripheral, not abort
    // solely because the HAL handle has not yet reached READY.
    testI2sInitReturnsError = true;
    assert(!backend.begin());
    assert(testI2sInitCount == 1);
    assert(testTransmitCount == 0);
    testI2sInitReturnsError = false;
    assert(backend.playTone(440, 350));
    assert(testI2sInitCount == 2);
    assert(testTransmitCount == 1);
    assert((SPI2->I2SCFGR & SPI_I2SCFGR_I2SMOD) != 0);
    assert((stubDma.CR & DMA_SxCR_EN) != 0);
    backend.stop();
    assert((stubDma.CR & DMA_SxCR_EN) == 0);

    // An unexpectedly enabled DMA must not be clobbered by recovery.
    SPI2->I2SCFGR = 0;
    SPI2->I2SPR = 2;
    stubDma.CR = DMA_SxCR_EN;
    assert(!backend.playTone(660, 100));
    assert(testI2sInitCount == 2);
    stubDma.CR = 0;
    assert(backend.playTone(660, 100));
    assert(testI2sInitCount == 3);
    backend.stop();

    bool recovered = false;
    for (const auto& message : debug.logs) {
        if (message.find("I2S2 recovery completed") != std::string::npos) {
            recovered = true;
        }
    }
    assert(recovered);
}
