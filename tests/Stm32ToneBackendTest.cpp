#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <stm32f4xx_hal.h>

#include "core/debug/IDebug.h"
#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

StubSd g_stm32Sd;
extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* handle);
extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* handle);

struct TestDebug final : IDebug {
    std::vector<std::string> logs;
    std::vector<std::string> errors;
    void log(const std::string& text) override { logs.push_back(text); }
    void error(const std::string& text) override { errors.push_back(text); }
};

int main() {
    TestDebug debug;
    Stm32F411AudioBackend backend({PB12, PB13, PB15}, &debug);
    assert(backend.begin());
    assert(testI2sInitCount == 1);
    assert((SPI2->I2SCFGR & SPI_I2SCFGR_I2SMOD) != 0);
    assert(!backend.playTone(0, 350));
    assert(!backend.playTone(440, 0));
    assert(backend.playTone(440, 350));
    assert(g_stm32Sd.openCount == 0);
    assert(testTransmitCount == 1);
    assert(testDmaBuffer != nullptr && testDmaLength == 4096);
    int lowest = 32767, highest = -32768;
    for (int i=0; i<1000; i++) {
        const std::int16_t left = static_cast<std::int16_t>(testDmaBuffer[i*2]);
        const std::int16_t right = static_cast<std::int16_t>(testDmaBuffer[i*2+1]);
        assert(left == right);
        if (left < lowest) lowest = left;
        if (left > highest) highest = left;
    }
    assert(lowest < -5900 && highest > 5900);

    // Simulate completed DMA buffer halves while the foreground loop refills them.
    I2S_HandleTypeDef handle{};
    handle.Instance = SPI2;
    for (int i=0; i<30; ++i) {
        if ((i % 2) == 0) HAL_I2S_TxHalfCpltCallback(&handle);
        else HAL_I2S_TxCpltCallback(&handle);
        backend.update();
    }
    bool finished = false;
    for (const auto& line : debug.logs) {
        if (line.find("playback finished") != std::string::npos) finished = true;
    }
    assert(finished);
    assert(g_stm32Sd.openCount == 0);
    // A peripheral reset by another HAL user must not result in silent playback.
    SPI2->I2SCFGR = 0;
    SPI2->I2SPR = 2;
    assert(backend.playTone(660, 100));
    assert(testI2sInitCount == 2);
    assert((SPI2->I2SCFGR & SPI_I2SCFGR_I2SMOD) != 0);
    backend.stop();
    assert(testTransmitCount == 2);
    assert(debug.errors.size() == 2); // only invalid tone requests are errors; recovery is logged

    // If HAL cannot restore I2S mode, playback must fail before starting DMA.
    SPI2->I2SCFGR = 0;
    SPI2->I2SPR = 2;
    testI2sInitFailsToConfigure = true;
    assert(!backend.playTone(880, 100));
    assert(testTransmitCount == 2);
    testI2sInitFailsToConfigure = false;
}
