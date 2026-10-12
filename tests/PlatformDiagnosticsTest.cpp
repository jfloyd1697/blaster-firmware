#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Arduino.h>

#include "core/Platform.h"
#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/lights/ILightBackend.h"
#include "core/lights/LightEngine.h"
#include "core/time/ITime.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "platform/stm32f411/Stm32F411Platform.h"
#include "platform/stm32f411/diagnostics/I2sGpioLoopback.h"

namespace {
unsigned long clockMs = 0;

class FakeTime final : public ITime {
public:
    unsigned long millis() const override { return clockMs; }
};

class FakeLightBackend final : public ILightBackend {
public:
    bool begin() override { return true; }
    std::size_t ledCount() const override { return 61; }
    void show(const std::vector<LightColor>& next) override {
        assert(next.size() == 61);
        pixels = next;
        ++frameUpdates;
    }
    std::vector<LightColor> pixels;
    unsigned frameUpdates = 0;
};

class SpyAudio final : public IAudioEngine {
public:
    bool begin() override { return true; }
    void update() override {}
    void playSound(const std::string& file, bool loop, bool blocking) override {
        (void) loop;
        (void) blocking;
        lastSound = file;
    }
    bool playTone(const std::uint32_t frequencyHz, const std::uint32_t durationMs) override {
        toneFrequencies.push_back(frequencyHz);
        assert(durationMs == 350 || durationMs == 5000);
        return true;
    }
    void stop() override { ++stopCount; }

    std::vector<std::uint32_t> toneFrequencies;
    std::string lastSound;
    int stopCount = 0;
};

class FileLoader final : public ITextResourceLoader {
public:
    const std::string fixtureRoot = "tests/data/compiled_assets/";

    std::string loadText(const std::string& path) override {
        std::ifstream file(fixtureRoot + path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    bool resourceExists(const std::string& path) const override {
        return std::filesystem::is_regular_file(fixtureRoot + path);
    }
    std::string readRange(const std::string& path, const std::size_t offset,
                          const std::size_t count) override {
        assert(count <= 8192);
        std::ifstream file(fixtureRoot + path, std::ios::binary);
        if (!file || !file.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string result(count, '\0');
        file.read(result.data(), static_cast<std::streamsize>(count));
        result.resize(static_cast<std::size_t>(file.gcount()));
        return result;
    }
};

class QuietDebug final : public IDebug {
public:
    void log(const std::string&) override {}
    void error(const std::string&) override {}
};

std::shared_ptr<SpyAudio> testAudio;
std::shared_ptr<FileLoader> testLoader;
FakeLightBackend* testLightBackend = nullptr;
}

DiagnosticTestSerial Serial;
unsigned long millis() { return clockMs; }
void delay(const unsigned long milliseconds) { clockMs += milliseconds; }

PlatformServices Stm32F411PlatformFactory::createConfiguredBoard() {
    testAudio = std::make_shared<SpyAudio>();
    testLoader = std::make_shared<FileLoader>();
    PlatformServices services;
    auto time = std::make_shared<FakeTime>();
    auto lightBackend = std::make_unique<FakeLightBackend>();
    testLightBackend = lightBackend.get();
    auto lights = std::make_shared<LightEngine>(std::move(lightBackend), time.get());
    assert(lights->begin());
    services.time = time;
    services.lights = lights;
    services.audio = testAudio;
    services.textLoader = testLoader;
    services.debug = std::make_shared<QuietDebug>();
    return services;
}

void setup();
void loop();

int main() {
    setup();
    assert(Serial.output.find("5 - generated tone test") != std::string::npos);
    assert(Serial.output.find("8 - WAV playback test") != std::string::npos);
    assert(Serial.output.find("0 - 61-LED light sequence showcase") != std::string::npos);
    // Menu display is always sorted numerically, independent of command dispatch.
    std::size_t previousPosition = 0;
    for (int command = 0; command <= 9; ++command) {
        const std::string label = std::to_string(command) + " - ";
        const std::size_t position = Serial.output.find(label);
        assert(position != std::string::npos);
        assert(position >= previousPosition);
        previousPosition = position + label.size();
    }


    // Test 5 repeats the three generated tones until the user stops it with q.
    Serial.input = "5";
    loop();
    assert((testAudio->toneFrequencies == std::vector<std::uint32_t>{440}));
    clockMs += 650;
    loop();
    clockMs += 650;
    loop();
    assert((testAudio->toneFrequencies == std::vector<std::uint32_t>{440, 660, 880}));
    clockMs += 650;
    loop();
    assert((testAudio->toneFrequencies == std::vector<std::uint32_t>{440, 660, 880, 440}));
    assert(Serial.output.find("Looping 440, 660, 880 Hz until q is pressed.") != std::string::npos);
    assert(testAudio->lastSound.empty());

    const int stopsBeforeQuit = testAudio->stopCount;
    Serial.input = "q";
    loop();
    assert(testAudio->stopCount == stopsBeforeQuit + 1);
    assert(Serial.output.find("=== Platform diagnostics ===") != std::string::npos);
    clockMs += 650;
    loop();
    assert(testAudio->toneFrequencies.size() == 4);

    // Re-entering the tone test should restart from the first tone.
    Serial.input = "5";
    loop();
    assert(testAudio->toneFrequencies.back() == 440);
    Serial.input = "q";
    loop();


    // Test a exercises GPIO/TIM2 loopback independently of the sound-producing
    // module, with a single wire moved between WS, BCLK and DATA.
    Serial.input = "a";
    loop();
    assert(stm32_i2s_loopback::running);
    assert(testAudio->toneFrequencies.back() == 440u);
    assert(Serial.output.find("PA15 (INPUT)") != std::string::npos);

    Serial.input = "t";
    loop();
    assert(Serial.output.find("PASS: PA15/TIM2 capture and jumper work") != std::string::npos);
    Serial.input = "s";
    loop();
    assert(Serial.output.find("I2S register snapshot only available") != std::string::npos);

    Serial.input = "w";
    loop();
    stm32_i2s_loopback::suppliedEdges = 11025u;
    clockMs += 250;
    loop();
    assert(Serial.output.find("WS: 43924 Hz | PASS") != std::string::npos);
    // A disconnected, pulled-down input must not appear as a valid clock.
    clockMs += 250;
    loop();
    assert(Serial.output.find("WS: 0 Hz | FAIL") != std::string::npos);

    Serial.input = "b";
    loop();
    stm32_i2s_loopback::suppliedEdges = 352800u;
    clockMs += 250;
    loop();
    assert(Serial.output.find("BCLK: 1405578 Hz | PASS") != std::string::npos);

    Serial.input = "d";
    loop();
    stm32_i2s_loopback::suppliedEdges = 43000u;
    clockMs += 250;
    loop();
    assert(Serial.output.find("DATA: 43000 rising edges / 251 ms | ACTIVITY") != std::string::npos);

    // Sustain the test past the five-second backend tone limit.
    const std::size_t toneCountBeforeRefresh = testAudio->toneFrequencies.size();
    clockMs += 4000;
    loop();
    assert(testAudio->toneFrequencies.size() == toneCountBeforeRefresh + 1);
    assert(testAudio->toneFrequencies.back() == 440u);

    const int stopsBeforeLoopbackQuit = testAudio->stopCount;
    Serial.input = "q";
    loop();
    assert(!stm32_i2s_loopback::running);
    assert(stm32_i2s_loopback::stops == 1);
    assert(testAudio->stopCount == stopsBeforeLoopbackQuit + 1);

    // Test 8 discovers a file through the compiled bank instead of using a stale path.
    Serial.input = "8";
    loop();
    assert(testAudio->lastSound == "assets/audio/fire.wav");
    Serial.input = "q";
    loop();
    assert(testAudio->stopCount >= 1);

    // Test 9 accepts an explicit file path and rejects nonexistent files.
    testAudio->lastSound.clear();
    Serial.input = "9";
    loop();
    Serial.input = "assets/audio/missing.wav\r";
    loop();
    assert(testAudio->lastSound.empty());
    assert(Serial.output.find("SD file does not exist") != std::string::npos);

    Serial.input = "9";
    loop();
    Serial.input = "assets/audio/fire.wav\r";
    loop();
    assert(testAudio->lastSound == "assets/audio/fire.wav");
    Serial.input = "q";
    loop();

    // Option 0 plays every compiled effect exactly once per cycle, then loops
    // the entire showcase until q is received. Verify actual frame output.
    const std::vector<std::string> effects = {
        "Solid", "Radial pulse", "Wipe", "Blink", "Chase", "Sparkle", "Rainbow"
    };
    Serial.input = "0";
    loop();
    assert(Serial.output.find("Each effect plays once; the full showcase repeats") != std::string::npos);
    assert(Serial.output.find("Light sequence 1/7: Solid") != std::string::npos);
    loop();
    assert(testLightBackend->pixels.size() == 61);
    const auto solidPixels = testLightBackend->pixels;
    assert(solidPixels[0] != LightColor({0, 0, 0}));
    for (std::size_t stageIndex = 1; stageIndex < effects.size(); ++stageIndex) {
        clockMs += 3000;
        loop();
        const auto label = "Light sequence " + std::to_string(stageIndex + 1) +
            "/7: " + effects[stageIndex];
        assert(Serial.output.find(label) != std::string::npos);
        // The newly selected effect is rendered on the following update.
        loop();
    }
    assert(testLightBackend->pixels != solidPixels);
    clockMs += 3000;
    loop();
    assert(Serial.output.find("Light sequence 1/7: Solid", Serial.output.find("Light sequence 7/7: Rainbow"))
           != std::string::npos);
    loop();
    assert(testLightBackend->pixels == solidPixels);

    const unsigned countBeforeStop = testLightBackend->frameUpdates;
    Serial.input = "q";
    loop();
    assert(testLightBackend->frameUpdates > countBeforeStop);
    for (const auto& pixel : testLightBackend->pixels) {
        assert(pixel == LightColor({0, 0, 0}));
    }
    clockMs += 5000;
    loop();
    assert(testLightBackend->pixels == std::vector<LightColor>(61, LightColor({0, 0, 0})));

    // Restarting always begins at the first effect.
    Serial.input = "0";
    loop();
    loop();
    assert(testLightBackend->pixels == solidPixels);
    Serial.input = "q";
    loop();
}
