#include <Arduino.h>
#include <array>
#include <cstdint>
#include <string>

#include "core/Platform.h"
#include "core/audio/IAudioEngine.h"
#include "core/assets/AssetLayoutValidator.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/lights/LightSequenceParser.h"
#include "core/storage/ResourceTreeDumper.h"
#include "core/weapons/WeaponsManifestParser.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"
#include "weapon_behavior/WeaponBehaviorParser.h"

#ifdef PLATFORM_ESP
#include "platform/esp8266/ESPPlatform.h"
#endif
#ifdef PLATFORM_TEENSY
#include "platform/teensy/TeensyPlatform.h"
#endif
#ifdef PLATFORM_STM32F411
#include "platform/stm32f411/Stm32F411Platform.h"
#include "platform/stm32f411/diagnostics/I2sGpioLoopback.h"
#if defined(ARDUINO_ARCH_STM32)
#include <stm32f4xx_hal.h>
#endif
#endif

namespace {
constexpr const char* kManifestPath = "assets/weapons/weapons_manifest.bin";
constexpr const char* kRainbowLayoutPath = "assets/lights/rainbow_ring_61.layout.bin";
constexpr std::uint16_t kShowcaseLedCount = 61;

struct ShowcaseStage {
    const char* name;
    const char* path;
};

// Match all six layer types supported by the light animation designer,
// then show the existing full-color rainbow. Every stage is a compiled
// WFLSEQ01 sequence streamed from SD, not an in-memory animation.
constexpr std::array<ShowcaseStage, 7> kShowcaseStages = {{{
    "Solid", "assets/lights/diagnostics/solid.bin",
}, {
    "Radial pulse", "assets/lights/diagnostics/radial_pulse.bin",
}, {
    "Wipe", "assets/lights/diagnostics/wipe.bin",
}, {
    "Blink", "assets/lights/diagnostics/blink.bin",
}, {
    "Chase", "assets/lights/diagnostics/chase.bin",
}, {
    "Sparkle", "assets/lights/diagnostics/sparkle.bin",
}, {
    "Rainbow", "assets/lights/rainbow_ring_61.bin",
}}};
constexpr std::array<std::uint32_t, 3> kToneFrequenciesHz = {440, 660, 880};
constexpr unsigned long kToneDurationMs = 350;
constexpr unsigned long kToneStepIntervalMs = 650;
constexpr std::size_t kMaxWavPathLength = 240;
constexpr unsigned long kLadderPrintIntervalMs = 100;
constexpr unsigned long kLightStepIntervalMs = 1000;
constexpr unsigned long kI2sMeasureIntervalMs = 250;
constexpr unsigned long kI2sToneRefreshMs = 4000;
constexpr unsigned long kI2sToneDurationMs = 5000;

enum class I2sLine {
    None,
    WordSelect,
    BitClock,
    Data,
};

enum class TestMode {
    Menu,
    Input,
    Ladder,
    Tone,
    SoundPath,
    Sound,
    Lights,
    LightShowcase,
    I2sLoopback,
};

PlatformServices services;
TestMode mode = TestMode::Menu;
unsigned long lastLadderPrintMs = 0;
unsigned long lastLightStepMs = 0;
unsigned long showcaseStartMs = 0;
std::uint32_t showcaseDurationMs = 0;
std::size_t showcaseStageIndex = 0;
std::size_t lightStep = 0;
std::size_t toneStep = 0;
unsigned long lastToneStepMs = 0;
std::string pendingWavPath;
I2sLine selectedI2sLine = I2sLine::None;
unsigned long i2sMeasurementStartMs = 0;
unsigned long i2sToneStartMs = 0;

PlatformServices createPlatform() {
#ifdef PLATFORM_ESP
    return ESPPlatformFactory::create();
#elif defined(PLATFORM_TEENSY)
    return TeensyPlatformFactory::create();
#elif defined(PLATFORM_STM32F411)
    return Stm32F411PlatformFactory::createConfiguredBoard();
#else
#error "Platform diagnostics require an embedded platform"
#endif
}

void printMenu() {
    Serial.println();
    Serial.println("=== Platform diagnostics ===");
    Serial.println("0 - 61-LED light sequence showcase (SD; loops until q)");
    Serial.println("1 - input test");
    Serial.println("2 - ladder test");
    Serial.println("3 - storage test");
    Serial.println("4 - storage tree dump");
    Serial.println("5 - generated tone test (no SD files or buttons required)");
    Serial.println("6 - lights test");
    Serial.println("7 - asset layout test");
    Serial.println("8 - WAV playback test (auto-select exported SD clip)");
    Serial.println("9 - WAV playback test (enter an existing SD path)");
    Serial.println("a - I2S GPIO loopback (STM32: PA15 input; WS/BCLK/DATA)");
    Serial.println("q - stop active test / return to menu");
    Serial.println();
}

void returnToMenu() {
    if ((mode == TestMode::Sound || mode == TestMode::Tone ||
         mode == TestMode::I2sLoopback) && services.audio) {
        services.audio->stop();
    }
    if ((mode == TestMode::Lights || mode == TestMode::LightShowcase) && services.lights) {
        services.lights->stopSequence();
        weapon_behavior::LightPatternDef off;
        off.mode = weapon_behavior::LightPatternMode::Solid;
        off.color = {0, 0, 0};
        off.brightness = 0;
        services.lights->setPattern(off);
        services.lights->update();
    }
#ifdef PLATFORM_STM32F411
    if (mode == TestMode::I2sLoopback) {
        stm32_i2s_loopback::end();
    }
#endif
    mode = TestMode::Menu;
    printMenu();
}

void runStorageTest() {
    Serial.println("=== Storage test ===");
    if (!services.textLoader) {
        Serial.println("FAIL: platform has no text resource loader");
        return;
    }

    const std::string text = services.textLoader->loadText(kManifestPath);
    if (text.empty()) {
        Serial.print("FAIL: could not load ");
        Serial.println(kManifestPath);
        return;
    }

    Serial.print("PASS: loaded ");
    Serial.print(kManifestPath);
    Serial.print(" (");
    Serial.print(static_cast<unsigned long>(text.size()));
    Serial.println(" bytes)");
}

void runSdDump() {
    Serial.println("=== Storage dump ===");
    if (!services.textLoader || !services.debug) {
        Serial.println("FAIL: platform has no resource loader/debug service");
        return;
    }

    ResourceTreeDumpOptions options;
    options.maxDepth = 4;
    options.maxEntriesPerDirectory = 64;
    ResourceTreeDumper::dump(*services.textLoader, *services.debug, "/", options);
}

void runAssetLayoutTest() {
    Serial.println("=== Asset layout test ===");
    if (!services.textLoader) {
        Serial.println("FAIL: platform has no text resource loader");
        return;
    }

    const AssetLayoutReport report = AssetLayoutValidator::validate(
        *services.textLoader,
        services.debug.get(),
        kManifestPath,
        AssetValidationDepth::ReferencesOnly);
    if (!report.ok) {
        Serial.print("FAIL: ");
        Serial.print(static_cast<unsigned long>(report.issues.size()));
        Serial.println(" issue(s)");
        for (const auto& issue : report.issues) {
            Serial.print("  ");
            Serial.print(issue.path.c_str());
            Serial.print(": ");
            Serial.println(issue.message.c_str());
        }
        return;
    }

    Serial.print("PASS: ");
    Serial.print(static_cast<unsigned long>(report.bankCount));
    Serial.print(" banks, ");
    Serial.print(static_cast<unsigned long>(report.weaponCount));
    Serial.print(" weapons, ");
    Serial.print(static_cast<unsigned long>(report.soundFileCount));
    Serial.println(" sound references");
}

void startToneTest() {
    Serial.println("=== Generated tone test ===");
    if (!services.audio) {
        Serial.println("FAIL: platform has no audio service");
        return;
    }
    mode = TestMode::Tone;
    toneStep = 0;
    lastToneStepMs = 0;
    Serial.println("Looping 440, 660, 880 Hz until q is pressed.");
    Serial.println("No SD card WAV, ladder, or button input is required.");
}

void updateToneTest() {
    const unsigned long now = millis();
    if (toneStep > 0 && now - lastToneStepMs < kToneStepIntervalMs) {
        return;
    }
    if (toneStep == kToneFrequenciesHz.size()) {
        toneStep = 0;
    }
    const std::uint32_t frequencyHz = kToneFrequenciesHz[toneStep];
    if (!services.audio->playTone(frequencyHz, kToneDurationMs)) {
        Serial.println("FAIL: generated tone unavailable or audio hardware not ready");
        returnToMenu();
        return;
    }
    Serial.print("Generated tone: ");
    Serial.print(frequencyHz);
    Serial.println(" Hz");
    lastToneStepMs = now;
    ++toneStep;
}

// Read the peripheral *while the diagnostic tone is transmitting*. This
// distinguishes an inactive peripheral/clock/DMA from a faulty external wire.
void printI2sHardwareStatus() {
#if defined(PLATFORM_STM32F411) && defined(ARDUINO_ARCH_STM32)
    const std::uint32_t i2sConfig = SPI2->I2SCFGR;
    const std::uint32_t dmaControl = DMA1_Stream4->CR;
    const std::uint32_t dmaBefore = DMA1_Stream4->NDTR;
    delayMicroseconds(100);
    const std::uint32_t dmaAfter = DMA1_Stream4->NDTR;
    const std::uint32_t modes = GPIOB->MODER;
    const std::uint32_t alternate = GPIOB->AFR[1];
    Serial.println("=== I2S2 hardware status (while tone runs) ===");
    Serial.print("PLLI2S ready: ");
    Serial.println((RCC->CR & RCC_CR_PLLI2SRDY) != 0u ? "YES" : "NO");
    Serial.print("I2S enabled: ");
    Serial.println((i2sConfig & SPI_I2SCFGR_I2SE) != 0u ? "YES" : "NO");
    Serial.print("I2S mode: ");
    Serial.println((i2sConfig & SPI_I2SCFGR_I2SMOD) != 0u ? "I2S" : "SPI");
    Serial.print("I2SCFGR raw: 0x");
    Serial.println(i2sConfig, HEX);
    Serial.print("I2S master TX: ");
    Serial.println((i2sConfig & SPI_I2SCFGR_I2SCFG) == I2S_MODE_MASTER_TX ? "YES" : "NO");
    Serial.print("I2S TX DMA request enabled: ");
    Serial.println((SPI2->CR2 & SPI_CR2_TXDMAEN) != 0u ? "YES" : "NO");
    Serial.print("DMA1 Stream4 enabled: ");
    Serial.println((dmaControl & DMA_SxCR_EN) != 0u ? "YES" : "NO");
    Serial.print("DMA NDTR before / after 100 us: ");
    Serial.print(dmaBefore);
    Serial.print(" / ");
    Serial.println(dmaAfter);
    Serial.print("DMA transferring: ");
    Serial.println(dmaBefore != dmaAfter ? "YES" : "NO (or between bursts)");
    Serial.print("SPI2 SR: 0x");
    Serial.println(SPI2->SR, HEX);
    Serial.print("I2S2 prescaler: 0x");
    Serial.println(SPI2->I2SPR, HEX);
    for (const std::uint32_t pin : std::array<std::uint32_t, 3>{12u, 13u, 15u}) {
        const std::uint32_t mode = (modes >> (pin * 2u)) & 3u;
        const std::uint32_t af = (alternate >> ((pin - 8u) * 4u)) & 15u;
        Serial.print("PB");
        Serial.print(pin);
        Serial.print(" mode (expected 2): ");
        Serial.print(mode);
        Serial.print(" AF (expected 5): ");
        Serial.println(af);
    }
#else
    Serial.println("I2S register snapshot only available on STM32 hardware");
#endif
}

void testI2sCounterReference() {
#ifdef PLATFORM_STM32F411
    Serial.println("Connect PB8 -> PA15 (REMOVE the PB12/PB13/PB15 jumper first).");
    const std::uint32_t count = stm32_i2s_loopback::runReferencePulseTest();
    Serial.print("Reference GPIO rising edges: ");
    Serial.print(count);
    Serial.println(" / 250 expected");
    Serial.println(count >= 245u && count <= 255u
        ? "PASS: PA15/TIM2 capture and jumper work"
        : "FAIL: check PB8->PA15 jumper and PA15 capture configuration");
    stm32_i2s_loopback::resetCounter();
    i2sMeasurementStartMs = millis();
#else
    Serial.println("Reference test unavailable on this platform");
#endif
}

void startI2sLoopbackTest() {
    Serial.println("=== STM32 I2S GPIO loopback ===");
#ifndef PLATFORM_STM32F411
    Serial.println("UNAVAILABLE: this test requires STM32F411 TIM2 input counting.");
#else
    if (!services.audio) {
        Serial.println("FAIL: no audio service");
        return;
    }
    if (!stm32_i2s_loopback::begin()) {
        Serial.println("FAIL: could not configure PA15/TIM2 edge counter");
        return;
    }
    if (!services.audio->playTone(440u, kI2sToneDurationMs)) {
        Serial.println("FAIL: I2S hardware did not start the diagnostic tone");
        stm32_i2s_loopback::end();
        return;
    }
    i2sToneStartMs = millis();
    selectedI2sLine = I2sLine::None;
    mode = TestMode::I2sLoopback;
    Serial.println("Connect one short jumper: PA15 (INPUT) -> one I2S output:");
    Serial.println("  w - PB12 (WS/LRCLK, expected approximately 44100 Hz)");
    Serial.println("  b - PB13 (BCLK, expected approximately 1411200 Hz)");
    Serial.println("  d - PB15 (DATA, edges expected; frequency not fixed)");
    Serial.println("  t - counter self-test: move jumper to PB8 -> PA15 (250 reference pulses)");
    Serial.println("  s - snapshot live I2S clocks, GPIO alternate functions and DMA");
    Serial.println("Move the jumper and press w/b/d to select what is connected.");
    printI2sHardwareStatus();
    Serial.println("Never short the three output pins together. Press q to stop.");
#endif
}

void selectI2sLine(const char command) {
#ifdef PLATFORM_STM32F411
    if (command == 's') {
        printI2sHardwareStatus();
        return;
    }
    if (command == 't') {
        selectedI2sLine = I2sLine::None;
        testI2sCounterReference();
        return;
    }
    switch (command) {
        case 'w':
            selectedI2sLine = I2sLine::WordSelect;
            Serial.println("Measuring WS: PA15 <- PB12");
            break;
        case 'b':
            selectedI2sLine = I2sLine::BitClock;
            Serial.println("Measuring BCLK: PA15 <- PB13");
            break;
        case 'd':
            selectedI2sLine = I2sLine::Data;
            Serial.println("Measuring DATA: PA15 <- PB15");
            break;
        default:
            Serial.println("Choose w, b, d, t, s, or q.");
            return;
    }
    stm32_i2s_loopback::resetCounter();
    i2sMeasurementStartMs = millis();
#else
    (void) command;
#endif
}

void updateI2sLoopbackTest() {
#ifdef PLATFORM_STM32F411
    const unsigned long now = millis();
    // Play a continuous 440 Hz source; refresh before the backend's maximum
    // five-second tone duration expires. Skip the counter window on refresh.
    if (now - i2sToneStartMs >= kI2sToneRefreshMs) {
        if (!services.audio->playTone(440u, kI2sToneDurationMs)) {
            Serial.println("FAIL: tone output stopped unexpectedly");
            returnToMenu();
            return;
        }
        i2sToneStartMs = now;
        stm32_i2s_loopback::resetCounter();
        i2sMeasurementStartMs = now;
    }
    if (selectedI2sLine == I2sLine::None ||
        now - i2sMeasurementStartMs < kI2sMeasureIntervalMs) {
        return;
    }
    const std::uint32_t elapsedMs = static_cast<std::uint32_t>(now - i2sMeasurementStartMs);
    const std::uint32_t edges = stm32_i2s_loopback::takeRisingEdges();
    i2sMeasurementStartMs = now;
    const std::uint32_t frequency = stm32_i2s_loopback::edgeFrequencyHz(edges, elapsedMs);

    switch (selectedI2sLine) {
        case I2sLine::WordSelect:
            Serial.print("WS: ");
            Serial.print(frequency);
            Serial.print(" Hz | ");
            Serial.println(stm32_i2s_loopback::nearFrequency(
                frequency, stm32_i2s_loopback::kWordSelectHz) ? "PASS" : "FAIL");
            break;
        case I2sLine::BitClock:
            Serial.print("BCLK: ");
            Serial.print(frequency);
            Serial.print(" Hz | ");
            Serial.println(stm32_i2s_loopback::nearFrequency(
                frequency, stm32_i2s_loopback::kBitClockHz) ? "PASS" : "FAIL");
            break;
        case I2sLine::Data:
            Serial.print("DATA: ");
            Serial.print(edges);
            Serial.print(" rising edges / ");
            Serial.print(elapsedMs);
            Serial.print(" ms | ");
            Serial.println(edges > 0u ? "ACTIVITY (not a framing check)" : "NO ACTIVITY");
            break;
        case I2sLine::None:
            break;
    }
#endif
}

void promptWavPath() {
    Serial.println("=== WAV file playback test ===");
    Serial.println("Enter the full path of a WAV already on the SD card, then press Enter.");
    Serial.println("Example: assets/weapons/Default/metroid_prime/audio/shot.wav");
    Serial.println("Press q to cancel.");
    pendingWavPath.clear();
    mode = TestMode::SoundPath;
}

void startWavPlayback(const std::string& path) {
    if (!services.audio || !services.textLoader) {
        Serial.println("FAIL: audio or SD resource loader not available");
        returnToMenu();
        return;
    }
    if (!services.textLoader->resourceExists(path)) {
        Serial.print("FAIL: SD file does not exist: ");
        Serial.println(path.c_str());
        returnToMenu();
        return;
    }
    Serial.print("Playing SD WAV: ");
    Serial.println(path.c_str());
    services.audio->playSound(path, false, false);
    mode = TestMode::Sound;
    Serial.println("Press q to stop and return to menu.");
}

void startAutomaticWavTest() {
    Serial.println("=== Exported WAV playback test ===");
    if (!services.audio || !services.textLoader) {
        Serial.println("FAIL: audio or SD resource loader not available");
        return;
    }

    const std::string manifestData = services.textLoader->loadText(kManifestPath);
    if (manifestData.empty()) {
        Serial.println("FAIL: cannot read compiled manifest; export SD assets first");
        return;
    }
    const WeaponsManifest manifest = WeaponsManifestParser::parseFromText(
        manifestData, services.debug.get());
    for (const auto& bank : manifest.banks) {
        for (const auto& weaponPath : bank.weapons) {
            const std::string runtimePath = "assets/weapons/" + weaponPath;
            if (!services.textLoader->resourceExists(runtimePath)) {
                continue;
            }
            // WFSMBIN1 puts its asset tables first. Read progressively and cap
            // memory usage, rather than loading a potentially large FSM into RAM.
            weapon_behavior::WeaponAssetReferences references;
            for (std::size_t headerBytes = 1024; headerBytes <= 8192; headerBytes *= 2) {
                const std::string header = services.textLoader->readRange(runtimePath, 0, headerBytes);
                references = weapon_behavior::WeaponBehaviorParser::parseAssetReferences(header, nullptr);
                if (references.valid || header.size() < headerBytes) {
                    break;
                }
            }
            if (!references.valid) {
                continue;
            }
            for (const auto& audioPath : references.audioPaths) {
                if (services.textLoader->resourceExists(audioPath)) {
                    startWavPlayback(audioPath);
                    return;
                }
            }
        }
    }
    Serial.println("FAIL: no existing WAV referenced by the exported weapon bank");
    Serial.println("Use option 9 to enter a different file path.");
}

void receiveWavPathCharacter(const char command) {
    if (command == '\r' || command == '\n') {
        if (!pendingWavPath.empty()) {
            const std::string path = pendingWavPath;
            pendingWavPath.clear();
            startWavPlayback(path);
        }
        return;
    }
    if (pendingWavPath.empty() && (command == 'q' || command == 'Q')) {
        returnToMenu();
        return;
    }
    if (command == '\b' || command == 127) {
        if (!pendingWavPath.empty()) {
            pendingWavPath.pop_back();
        }
        return;
    }
    if (static_cast<unsigned char>(command) >= 32 &&
        pendingWavPath.size() < kMaxWavPathLength) {
        pendingWavPath.push_back(command);
    }
}

weapon_behavior::LightPatternDef solid(const std::array<int, 3>& color) {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Solid;
    pattern.color = color;
    pattern.brightness = 128;
    return pattern;
}

void startLightsTest() {
    Serial.println("=== Lights test ===");
    if (!services.lights) {
        Serial.println("FAIL: platform has no lights service");
        return;
    }
    mode = TestMode::Lights;
    lightStep = 0;
    lastLightStepMs = 0;
    Serial.println("Cycling red, green, blue, white. Press q to stop.");
}

bool startShowcaseStage() {
    const auto& stage = kShowcaseStages[showcaseStageIndex];
    LightSequenceHeader header;
    if (!LightSequenceParser::readHeader(*services.textLoader, stage.path, header) ||
        header.ledCount != kShowcaseLedCount ||
        !services.lights->playSequenceFile(stage.path, *services.textLoader, LightSequencePlayback{true})) {
        Serial.print("FAIL: could not stream light effect: ");
        Serial.println(stage.path);
        return false;
    }
    showcaseDurationMs = header.totalDurationMs;
    showcaseStartMs = millis();
    Serial.print("Light sequence ");
    Serial.print(static_cast<unsigned>(showcaseStageIndex + 1));
    Serial.print("/");
    Serial.print(static_cast<unsigned>(kShowcaseStages.size()));
    Serial.print(": ");
    Serial.println(stage.name);
    return true;
}

void startLightShowcaseTest() {
    Serial.println("=== 61-LED light sequence showcase ===");
    if (!services.lights || !services.textLoader) {
        Serial.println("FAIL: lights or SD resource loader unavailable");
        return;
    }

    std::uint16_t layoutLeds = 0;
    if (!LightSequenceParser::readLayoutHeader(*services.textLoader, kRainbowLayoutPath, layoutLeds) ||
        layoutLeds != kShowcaseLedCount) {
        Serial.print("FAIL: missing/invalid 61-LED layout: ");
        Serial.println(kRainbowLayoutPath);
        return;
    }
    // Fail fast with an actionable path instead of stopping midway through
    // the showcase. Each check reads only the fixed-size binary header.
    for (const auto& stage : kShowcaseStages) {
        LightSequenceHeader header;
        if (!LightSequenceParser::readHeader(*services.textLoader, stage.path, header) ||
            header.ledCount != layoutLeds) {
            Serial.print("FAIL: missing/invalid compiled light sequence: ");
            Serial.println(stage.path);
            return;
        }
    }

    showcaseStageIndex = 0;
    Serial.println("Cycling solid, radial pulse, wipe, blink, chase, sparkle, rainbow.");
    Serial.println("Each effect plays once; the full showcase repeats until q is pressed.");
    if (!startShowcaseStage()) {
        return;
    }
    mode = TestMode::LightShowcase;
}

void updateLightShowcaseTest() {
    if (millis() - showcaseStartMs < showcaseDurationMs) {
        return;
    }
    showcaseStageIndex = (showcaseStageIndex + 1) % kShowcaseStages.size();
    if (!startShowcaseStage()) {
        returnToMenu();
    }
}

void updateInputTest() {
    if (!services.input) {
        return;
    }
    if (services.input->wasTriggerPressed()) Serial.println("Trigger pressed");
    if (services.input->wasTriggerReleased()) Serial.println("Trigger released");
    if (services.input->wasNextShortPressed()) Serial.println("Next short");
    if (services.input->wasNextLongPressed()) Serial.println("Next long");
    if (services.input->wasPrevShortPressed()) Serial.println("Previous short");
    if (services.input->wasPrevLongPressed()) Serial.println("Previous long");
    if (services.input->wasReloadPressed()) Serial.println("Reload pressed");
    if (services.input->wasQuitPressed()) Serial.println("Quit pressed");
}

void updateLadderTest() {
    const unsigned long now = millis();
    if (now - lastLadderPrintMs < kLadderPrintIntervalMs) {
        return;
    }
    lastLadderPrintMs = now;

    if (!services.input) {
        Serial.println("FAIL: platform has no input service");
        returnToMenu();
        return;
    }

    const auto value = services.input->rawLadderValue();
    if (!value.has_value()) {
        Serial.println("FAIL: this input implementation does not expose a ladder reading");
        returnToMenu();
        return;
    }

    Serial.print("ladder = ");
    Serial.println(*value);
}

void updateLightsTest() {
    if (!services.lights) {
        return;
    }
    const unsigned long now = millis();
    if (lastLightStepMs != 0 && now - lastLightStepMs < kLightStepIntervalMs) {
        return;
    }
    lastLightStepMs = now;

    constexpr std::array<std::array<int, 3>, 4> colors = {{
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 255},
    }};
    services.lights->setPattern(solid(colors[lightStep]));
    lightStep = (lightStep + 1) % colors.size();
}

void handleCommand(char command) {
    if (mode == TestMode::SoundPath) {
        receiveWavPathCharacter(command);
        return;
    }
    if (command == '\r' || command == '\n') {
        return;
    }
    if (command == 'q' || command == 'Q') {
        returnToMenu();
        return;
    }
    if (mode == TestMode::I2sLoopback) {
        selectI2sLine(command);
        return;
    }
    if (mode != TestMode::Menu) {
        return;
    }

    switch (command) {
        case 'a':
        case 'A':
            startI2sLoopbackTest();
            break;
        case '1':
            mode = TestMode::Input;
            Serial.println("=== Input test ===");
            Serial.println("Operate the physical controls. Press q to stop.");
            break;
        case '2':
            mode = TestMode::Ladder;
            lastLadderPrintMs = 0;
            Serial.println("=== Ladder test ===");
            Serial.println("Press ladder buttons and observe the raw ADC value. Press q to stop.");
            break;
        case '3':
            runStorageTest();
            printMenu();
            break;
        case '4':
            runSdDump();
            printMenu();
            break;
        case '5':
            startToneTest();
            break;
        case '8':
            startAutomaticWavTest();
            break;
        case '9':
            promptWavPath();
            break;
        case '6':
            startLightsTest();
            break;
        case '0':
            startLightShowcaseTest();
            break;
        case '7':
            runAssetLayoutTest();
            break;
        default:
            printMenu();
            break;
    }
}
}

void setup() {
    services = createPlatform();
    Serial.println();
    Serial.println("Platform diagnostics ready");
    printMenu();
}

void loop() {
    if (services.input) {
        services.input->update();
    }
    if (services.audio) {
        services.audio->update();
    }
    if (services.lights) {
        services.lights->update();
    }

    while (Serial.available() > 0) {
        handleCommand(static_cast<char>(Serial.read()));
    }

    switch (mode) {
        case TestMode::Input:
            updateInputTest();
            break;
        case TestMode::Ladder:
            updateLadderTest();
            break;
        case TestMode::Lights:
            updateLightsTest();
            break;
        case TestMode::Tone:
            updateToneTest();
            break;
        case TestMode::LightShowcase:
            updateLightShowcaseTest();
            break;
        case TestMode::I2sLoopback:
            updateI2sLoopbackTest();
            break;
        case TestMode::Menu:
        case TestMode::SoundPath:
        case TestMode::Sound:
            break;
    }

    delay(1);
}
