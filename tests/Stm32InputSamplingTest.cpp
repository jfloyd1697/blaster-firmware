#include <cassert>
#include <cstdint>

#include "platform/stm32f411/input/Stm32F411Input.h"
#include "core/time/ITime.h"

namespace {
int analogReads = 0;
int latestSample = 4095;
int analogBits = 0;
class FakeTime final : public ITime {
public:
    unsigned long millis() const override { return now; }
    unsigned long now = 0;
};
}

void pinMode(std::uint32_t, int) {}
int digitalRead(std::uint32_t) { return HIGH; }
int analogRead(std::uint32_t) { ++analogReads; return latestSample; }
void analogReadResolution(int bits) { analogBits = bits; }

int main() {
    FakeTime clock;
    Stm32F411Input input(Stm32F411InputPins{1u, 2u}, &clock);
    input.begin();
    assert(analogBits == 12);
    for (int cycle = 0; cycle < 100; ++cycle) {
        clock.now = static_cast<unsigned long>(cycle);
        latestSample = cycle < 50 ? 90 : 4095;
        input.update();
        assert(analogReads == cycle + 1); // one conversion for three ladder buttons
    }
    // A manually requested diagnostic reading remains fresh.
    latestSample = 400;
    assert(input.rawLadderValue().value() == 400);
    assert(analogReads == 101);
    return 0;
}
