#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "core/debug/IDebug.h"
#include "core/time/ITime.h"
#include "core/weapons/ShootContext.h"
#include "core/weapons/WeaponBehaviorController.h"
#include "weapon_behavior/WeaponBehaviorParser.h"
#include "weapon_behavior/WeaponBehaviorValidation.h"

namespace {
struct TestDebug final : IDebug {
    void log(const std::string&) override {}
    void error(const std::string&) override {}
};

class TestTime final : public ITime {
public:
    [[nodiscard]] unsigned long millis() const override { return value; }
    unsigned long value = 0;
};
}

int main() {
    std::ifstream input("tests/data/simple_weapon.bin", std::ios::binary);
    assert(input.good());
    const std::string data(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());

    TestDebug debug;
    TestTime time;
    assert(weapon_behavior::WeaponBehaviorParser::isBinaryFormat(data));
    const auto references = weapon_behavior::WeaponBehaviorParser::parseAssetReferences(data, &debug);
    assert(references.valid);
    assert(references.audioPaths.size() == 1);
    assert(references.audioPaths.front() == "assets/fire.wav");

    auto behavior = weapon_behavior::WeaponBehaviorParser::parse(data, &debug);
    assert(weapon_behavior::validateWeaponBehavior(behavior).ok);

    std::vector<std::string> played;
    ShootContext context;
    context.debug = &debug;
    context.time = &time;
    context.playAudio = [&](const std::string& path, bool, const std::string&) { played.push_back(path); };

    WeaponBehaviorController controller(behavior, context);
    controller.initialize();
    controller.handleEvent("trigger_pressed");
    assert(controller.currentState() == "firing");
    assert(played.size() == 1 && played.front() == "assets/fire.wav");
    assert(std::get<std::int64_t>(controller.variables().at("ammo")) == 1);

    time.value = 10;
    controller.update();
    assert(controller.currentState() == "ready");

    // The firmware validator must reject invalid references before equipping.
    weapon_behavior::ActionDef badRandom;
    badRandom.type = "play_random_audio";
    badRandom.clips = {"missing_sound"};
    behavior.states.begin()->second.onEntry.push_back(badRandom);
    const auto invalid = weapon_behavior::validateWeaponBehavior(behavior);
    assert(!invalid.ok);
    bool foundMissingClip = false;
    for (const auto& issue : invalid.errors) {
        if (issue.find("unknown clip") != std::string::npos) {
            foundMissingClip = true;
        }
    }
    assert(foundMissingClip);
    return 0;
}
