#include "lights/animation/Animation.h"

namespace Lights::Animations {

    Blink::Blink(int r, int g, int b)
        : r_(r), g_(g), b_(b)
    {}

    void Blink::step(std::vector<LED>& leds) {
        for (auto& led : leds) {
            if (on_) {
                led.r = r_;
                led.g = g_;
                led.b = b_;
            } else {
                led.r = led.g = led.b = 0;
            }
        }
        on_ = !on_;
    }

} // namespace Lights::Animations