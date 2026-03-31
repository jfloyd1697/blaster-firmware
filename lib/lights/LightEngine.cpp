#include "Animation.h"
#include "LightEngine.h"

namespace Lights {

    LedLights::LedLights(size_t numLEDs)
        : leds_(numLEDs)
    {}

    void LedLights::update() {
        for (auto& anim : animations_) {
            anim->step(leds_);
        }
    }

    void LedLights::addAnimation(std::shared_ptr<Animations::IAnimation> anim) {
        animations_.push_back(anim);
    }

    const std::vector<LED>& LedLights::getLEDs() const {
        return leds_;
    }

    std::shared_ptr<ILights> createLights(size_t numLEDs) {
        return std::make_shared<LedLights>(numLEDs);
    }

} // namespace Lights