#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "core/Blaster.h"
#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/display/IStatusDisplay.h"
#include "core/lights/ILights.h"
#include "core/lights/ILightBackend.h"
#include "core/lights/LightEngine.h"
#include "core/lights/LightSequenceParser.h"
#include "core/input/IInput.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/time/ITime.h"

namespace {
class TestDebug final : public IDebug {
public:
    void log(const std::string&) override {}
    void error(const std::string& value) override { errors.push_back(value); }
    std::vector<std::string> errors;
};

class TestTime final : public ITime {
public:
    unsigned long millis() const override { return now; }
    unsigned long now = 0;
};

class TestAudio final : public IAudioEngine {
public:
    explicit TestAudio(std::vector<std::string>& records) : records(records) {}
    bool begin() override { return true; }
    void update() override {}
    void playSound(const std::string& path, bool, bool) override {
        records.push_back("audio:start:" + path);
    }
    void stop() override { records.push_back("audio:stop"); }
    bool prepareSounds(const std::vector<std::string>& paths) override {
        ++prepareCount;
        preparedPaths = paths;
        return true;
    }
    void releasePreparedSounds() override {
        ++releaseCount;
        preparedPaths.clear();
    }
    int prepareCount = 0;
    int releaseCount = 0;
    std::vector<std::string> preparedPaths;
    std::vector<std::string>& records;
};

class TestDisplay final : public IStatusDisplay {
public:
    void present(const DisplayStatus& value) override { last = value; ++updates; }
    void setControlMode(DisplayControlMode) override {}
    void service(std::uint32_t) override {}
    DisplayStatus last;
    int updates = 0;
};

class TestLights final : public ILights {
public:
    explicit TestLights(std::vector<std::string>& records) : records(records) {}
    void setPattern(const weapon_behavior::LightPatternDef&) override {}
    void flashPattern(const weapon_behavior::LightPatternDef&) override {}
    void flash() override {}
    void playSequence(const LightSequence&, bool) override {}
    bool playSequenceFile(const std::string& path, ITextResourceLoader&, LightSequencePlayback playback) override {
        records.push_back("light:start:" + path + ":" + std::to_string(playback.firstFrame));
        return true;
    }
    bool requestSequenceFile(const std::string& path, ITextResourceLoader& loader,
                             LightSequencePlayback playback) override {
        return playSequenceFile(path, loader, playback);
    }
    void stopSequence() override { records.push_back("light:stop"); }
    void update() override {}
    std::vector<std::string>& records;
};

class PhysicalLedBackend final : public ILightBackend {
public:
    bool begin() override { return true; }
    std::size_t ledCount() const override { return 61; }
    void show(const std::vector<LightColor>& value) override { pixels = value; }
    std::vector<LightColor> pixels;
};

class TestInput final : public IInput {
public:
    explicit TestInput(ITime* time) : IInput(time) {}
    bool readRawButton(ButtonID) const override { return false; }
};

class FileLoader final : public ITextResourceLoader {
public:
    std::string loadText(const std::string& path) override {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    bool resourceExists(const std::string& path) const override {
        std::ifstream file(path, std::ios::binary);
        return file.good();
    }
    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        std::ifstream file(path, std::ios::binary);
        if (!file || !file.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string buffer(count, '\0');
        file.read(buffer.data(), static_cast<std::streamsize>(count));
        buffer.resize(static_cast<std::size_t>(file.gcount()));
        return buffer;
    }
};

class TestBlaster final : public Blaster {
public:
    using Blaster::Blaster;
    using Blaster::selectNextWeapon;
};
}

int main() {
    std::vector<std::string> log;
    PlatformServices services;
    services.audio = std::make_shared<TestAudio>(log);
    services.lights = std::make_shared<TestLights>(log);
    services.debug = std::make_shared<TestDebug>();
    services.display = std::make_shared<TestDisplay>();
    services.time = std::make_shared<TestTime>();
    services.textLoader = std::make_shared<FileLoader>();

    WeaponBank bank;
    bank.name = "Test";
    bank.weapons = {
        {"Prime", "assets/weapons/Default/metroid_prime/weapon.bin"},
        {"C3-PO", "assets/weapons/Characters/c3p0_hurt/weapon.bin"},
        {"Yoda", "assets/weapons/Characters/yoda_hurt/weapon.bin"},
        {"Darth Maul", "assets/weapons/Characters/darth_maul_hurt/weapon.bin"},
        {"Missing", "assets/weapons/Default/does_not_exist/weapon.bin"},
    };
    auto audio = std::static_pointer_cast<TestAudio>(services.audio);
    TestBlaster blaster(services, {bank});
    auto display = std::static_pointer_cast<TestDisplay>(services.display);
    assert(display->last.weaponName == "Prime");
    assert(display->last.bankName == "Test");
    assert(display->last.weaponState == DisplayWeaponState::Ready);
    assert(audio->prepareCount == 1);
    assert(audio->releaseCount == 1);
    assert(!audio->preparedPaths.empty());
    assert(std::all_of(audio->preparedPaths.begin(), audio->preparedPaths.end(),
        [](const std::string& path) { return path.size() > 4 && path.substr(path.size() - 4) == ".wav"; }));
    assert(std::any_of(log.begin(), log.end(), [](const std::string& value) {
        return value.rfind("audio:start:", 0) == 0;
    }));
    assert(std::any_of(log.begin(), log.end(), [](const std::string& value) {
        return value.rfind("light:start:", 0) == 0;
    }));

    log.clear();
    blaster.selectNextWeapon();
    assert(display->last.weaponName == "C3-PO");
    // On equip, both outputs must be stopped BEFORE starting the new effects.
    assert(log.size() >= 4);
    assert(log[0] == "audio:stop");
    assert(log[1] == "light:stop");
    assert(std::any_of(log.begin() + 2, log.end(), [](const std::string& value) {
        return value.rfind("audio:start:", 0) == 0;
    }));
    assert(std::any_of(log.begin() + 2, log.end(), [](const std::string& value) {
        return value.rfind("light:start:", 0) == 0;
    }));

    // Curated character profiles use an on_equip event (rather than an equip
    // initial state). Both must start their own transition sound and wipe.
    for (const std::string& character : {"Yoda", "Darth Maul"}) {
        (void)character;
        log.clear();
        blaster.selectNextWeapon();
        assert(log.size() >= 4);
        assert(log[0] == "audio:stop");
        assert(log[1] == "light:stop");
        assert(std::any_of(log.begin() + 2, log.end(), [](const std::string& value) {
            return value.rfind("audio:start:", 0) == 0;
        }));
        assert(std::any_of(log.begin() + 2, log.end(), [](const std::string& value) {
            return value.rfind("light:start:", 0) == 0;
        }));
    }

    log.clear();
    const int beforeInvalidPrepare = audio->prepareCount;
    const int beforeInvalidRelease = audio->releaseCount;
    blaster.selectNextWeapon();
    // An invalid weapon must also silence the previous one.
    assert(audio->releaseCount == beforeInvalidRelease + 1);
    assert(audio->prepareCount == beforeInvalidPrepare);
    assert(audio->preparedPaths.empty());
    assert(display->last.weaponName == "Missing");
    assert(display->last.weaponState == DisplayWeaponState::Error);
    assert(log.size() == 2);
    assert(log[0] == "audio:stop");
    assert(log[1] == "light:stop");
    blaster.selectNextWeapon();
    assert(log.size() >= 4);
    assert(audio->prepareCount == beforeInvalidPrepare + 1);
    assert(!audio->preparedPaths.empty());

    // The 20-stud interaction comes exclusively from the compiled weapon FSM and light sequences.
    auto clock = std::make_shared<TestTime>();
    auto input = std::make_shared<TestInput>(clock.get());
    input->setRemoteInputEnabled(true);
    services.time = clock;
    services.input = input;
    WeaponBank jediBank;
    jediBank.name = "Characters";
    jediBank.weapons = {{"True Jedi", "assets/weapons/Characters/true_jedi/weapon.bin"}};
    TestBlaster jedi(services, {jediBank});
    assert(display->last.behaviorState == "STUDS 00/20");
    input->update(); // Establish released debounced baseline before the first press.
    FileLoader loader;
    const std::string sequenceRoot = "assets/weapons/Characters/true_jedi/lights/";
    const auto sequence = sequenceRoot + "stud_progress.bin";
    assert(LightSequenceParser::validateBinary(loader, sequence));
    assert(LightSequenceParser::validateLayout(loader, sequenceRoot + "stud_progress.layout.bin"));
    LightSequenceHeader header;
    assert(LightSequenceParser::readHeader(loader, sequence, header));
    assert(header.ledCount == 61);
    assert(header.frameCount == 26); // 20 static progress frames + 6 completion frames.
    auto tick = [&](ButtonID button, bool pressed) {
        input->setRemoteButton(button, pressed);
        clock->now += 21;
        input->update();
        jedi.update();
        clock->now += 21;
        input->update();
        jedi.update();
    };
    // Like AIAI Fallout, the progress is driven by scheduled FSM events
    // while the trigger is held, never by twenty separate button presses.
    auto advance = [&](const unsigned long elapsedMs) {
        clock->now += elapsedMs;
        input->update();
        jedi.update();
    };

    // Release early: the pending progress event must be cancelled.
    tick(ButtonID::Trigger, true);
    assert(display->last.behaviorState == "STUDS 01/20");
    advance(400);
    assert(display->last.behaviorState == "STUDS 02/20");
    tick(ButtonID::Trigger, false);
    assert(display->last.behaviorState == "STUDS 00/20");
    advance(1600);
    assert(display->last.behaviorState == "STUDS 00/20");

    log.clear();
    tick(ButtonID::Trigger, true);
    assert(display->last.behaviorState == "STUDS 01/20");
    advance(399);
    assert(display->last.behaviorState == "STUDS 01/20");
    for (int stage = 2; stage <= 20; ++stage) {
        advance(stage == 2 ? 1 : 400);
        const std::string label = std::string("STUDS ") + (stage < 10 ? "0" : "") +
            std::to_string(stage) + "/20";
        assert(display->last.behaviorState == label);
    }
    assert(std::any_of(log.begin(), log.end(), [](const std::string& item) {
        return item == "light:start:assets/weapons/Characters/true_jedi/lights/stud_progress.bin:20";
    }));
    // All stages reference the same compiled sequence, selecting one frame each.
    for (int i = 1; i < 21; ++i) {
        assert(std::any_of(log.begin(), log.end(), [i](const std::string& item) {
            return item == "light:start:assets/weapons/Characters/true_jedi/lights/stud_progress.bin:" +
                std::to_string(i);
        }));
    }
    // Completion plays through even if the trigger is released, then resets
    // automatically just as the Fallout charge-shot returns to ready.
    tick(ButtonID::Trigger, false);
    advance(1400);
    assert(display->last.behaviorState == "STUDS 20/20");
    advance(100);
    assert(display->last.behaviorState == "STUDS 00/20");
    tick(ButtonID::Trigger, true);
    assert(display->last.behaviorState == "STUDS 01/20");
    tick(ButtonID::Trigger, false);
    tick(ButtonID::Reload, true);
    tick(ButtonID::Reload, false);
    assert(display->last.behaviorState == "STUDS 00/20");

    // Exercise the real streamed binary renderer, not just the mocked FSM requests.
    TestTime renderClock;
    auto physical = std::make_unique<PhysicalLedBackend>();
    auto* pixels = physical.get();
    LightEngine engine(std::move(physical), &renderClock);
    assert(engine.begin());
    const auto select = [&](const std::size_t frame, const bool hold, const bool loop) {
        engine.requestSequenceFile(sequence, loader, LightSequencePlayback{loop, frame, hold});
        // First header read, then up to `frame` bounded metadata seeks, then the one pixel read.
        for (std::size_t step = 0; step < frame + 2; ++step) engine.prefetchNextFrame();
        engine.update();
    };
    select(0, true, false);
    assert((pixels->pixels[0] == LightColor{10, 3, 0}));
    select(10, true, false);
    assert((pixels->pixels[0] == LightColor{255, 205, 0}));
    assert((pixels->pixels[30] == LightColor{10, 3, 0}));
    renderClock.now += 120000; // Held frame never advances with time.
    engine.update();
    assert((pixels->pixels[30] == LightColor{10, 3, 0}));
    select(19, true, false);
    assert((pixels->pixels[56] == LightColor{255, 205, 0}));
    assert((pixels->pixels[58] == LightColor{10, 3, 0}));

    select(20, false, true);
    assert((pixels->pixels[0] == LightColor{255, 120, 0}));
    for (int step = 1; step <= 6; ++step) {
        engine.prefetchNextFrame();
        renderClock.now += 90;
        engine.update();
        if (step == 1) assert((pixels->pixels[0] == LightColor{255, 170, 0}));
    }
    assert((pixels->pixels[0] == LightColor{255, 120, 0}));
    return 0;
}
