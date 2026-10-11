#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "core/lights/ILightBackend.h"
#include "core/lights/LightEngine.h"
#include "core/time/ITime.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class FakeTime final : public ITime {
public:
    unsigned long millis() const override { return now; }
    unsigned long now = 0;
};

class FakeLightBackend final : public ILightBackend {
public:
    bool begin() override { began = true; return true; }
    std::size_t ledCount() const override { return 2; }
    void show(const std::vector<LightColor>& value) override { pixels = value; }

    bool began = false;
    std::vector<LightColor> pixels;
};

int main() {
    FakeTime time;
    auto backend = std::make_unique<FakeLightBackend>();
    auto* rawBackend = backend.get();
    LightEngine lights(std::move(backend), &time);
    assert(lights.begin());
    assert(rawBackend->began);

    weapon_behavior::LightPatternDef solid;
    solid.mode = weapon_behavior::LightPatternMode::Solid;
    solid.color = {100, 50, 0};
    solid.brightness = 255;
    lights.setPattern(solid);
    lights.update();
    assert(rawBackend->pixels.size() == 2);
    assert((rawBackend->pixels[0] == LightColor{100, 50, 0}));

    LightSequence sequence;
    sequence.ledCount = 2;
    sequence.frames.push_back(LightFrame{100, {LightPixel{1, {0, 100, 0}, 1.0F}}});
    lights.playSequence(sequence, false);
    lights.update();
    assert((rawBackend->pixels[0] == LightColor{0, 0, 0}));
    assert((rawBackend->pixels[1] == LightColor{0, 100, 0}));
    return 0;
}
