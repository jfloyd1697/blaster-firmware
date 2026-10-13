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

class FileLoader final : public ITextResourceLoader {
public:
    std::string loadText(const std::string& path) override {
        std::ifstream file(std::string("sdcard/") + path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    bool resourceExists(const std::string& path) const override {
        std::ifstream file(std::string("sdcard/") + path, std::ios::binary);
        return file.good();
    }
    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        std::ifstream file(std::string("sdcard/") + path, std::ios::binary);
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

    return 0;
}
