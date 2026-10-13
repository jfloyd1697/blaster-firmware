#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <stm32f4xx_hal.h>

#include "core/debug/IDebug.h"
#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

StubSd g_stm32Sd;
extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef*);
extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef*);

struct TestDebug final : IDebug {
    std::vector<std::string> errors;
    void log(const std::string&) override {}
    void error(const std::string& text) override { errors.push_back(text); }
};

namespace {
void write16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void write32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    write16(bytes, static_cast<std::uint16_t>(value));
    write16(bytes, static_cast<std::uint16_t>(value >> 16));
}

std::vector<std::uint8_t> makeWav() {
    std::vector<std::uint8_t> result{'R', 'I', 'F', 'F'};
    write32(result, 36 + 4096);
    result.insert(result.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    write32(result, 16);
    write16(result, 1);       // PCM
    write16(result, 1);       // Mono
    write32(result, 44100);
    write32(result, 88200);
    write16(result, 2);       // Block align
    write16(result, 16);      // Bits per sample
    result.insert(result.end(), {'d', 'a', 't', 'a'});
    write32(result, 4096);
    for (int i = 0; i < 2048; ++i) {
        write16(result, static_cast<std::uint16_t>(i));
    }
    return result;
}
}

int main() {
    g_stm32Sd.addFile("assets/a.wav", makeWav());
    g_stm32Sd.addFile("assets/b.wav", makeWav());

    TestDebug debug;
    Stm32F411AudioBackend backend({PB12, PB13, PB15}, &debug);
    assert(backend.begin());

    assert(backend.prepareSounds({"assets/a.wav", "assets/b.wav", "assets/a.wav"}));
    assert(g_stm32Sd.openCount == 2); // All unique WAVs opened once on equip.

    backend.playSound("assets/a.wav", false, false);
    assert(testTransmitCount == 0); // Start request does not touch SD or start DMA.
    backend.update();
    assert(testTransmitCount == 1);
    assert(g_stm32Sd.openCount == 2); // No open/header parsing on trigger.
    backend.playSound("assets/b.wav", false, false);
    backend.update();
    assert(testTransmitCount == 2);
    assert(g_stm32Sd.openCount == 2);
    backend.playSound("assets/a.wav", false, false);
    backend.update();
    assert(testTransmitCount == 3);
    assert(g_stm32Sd.openCount == 2); // Repeated shots reuse the same handle.

    backend.stop(); // Stopping audio does not discard prepared clips.
    backend.playSound("assets/b.wav", true, false);
    backend.update();
    assert(testTransmitCount == 4);
    assert(g_stm32Sd.openCount == 2);

    backend.releasePreparedSounds(); // Weapon switching drops old open handles.
    backend.playSound("assets/a.wav", false, false);
    backend.update();
    assert(testTransmitCount == 5);
    assert(g_stm32Sd.openCount == 3); // Unprepared file takes legacy fallback.
    backend.stop();

    // DMA completion closes only the active stream, not the prepared file.
    // It must still be available when the next shot fires.
    assert(backend.prepareSounds({"assets/a.wav"}));
    const int opensBeforeCompletion = g_stm32Sd.openCount;
    backend.playSound("assets/a.wav", false, false);
    I2S_HandleTypeDef handle{};
    handle.Instance = SPI2;
    HAL_I2S_TxHalfCpltCallback(&handle);
    backend.update();
    HAL_I2S_TxCpltCallback(&handle);
    backend.update();
    HAL_I2S_TxHalfCpltCallback(&handle);
    backend.update();
    backend.playSound("assets/a.wav", false, false);
    assert(g_stm32Sd.openCount == opensBeforeCompletion);
    backend.stop();

    assert(!backend.prepareSounds({"assets/a.wav", "assets/missing.wav"}));
    assert(g_stm32Sd.openCount == 6);
    assert(debug.errors.size() == 1);
    backend.playSound("assets/a.wav", false, false);
    assert(g_stm32Sd.openCount == 6);
    backend.releasePreparedSounds();
}
