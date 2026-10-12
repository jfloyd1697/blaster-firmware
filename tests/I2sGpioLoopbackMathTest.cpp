#include <cassert>
#include <cstdint>

#include "platform/stm32f411/diagnostics/I2sGpioLoopback.h"

int main() {
    using namespace stm32_i2s_loopback;
    assert(kWordSelectHz == 44100u);
    assert(kBitClockHz == 1411200u);
    assert(edgeFrequencyHz(11025u, 250u) == 44100u);
    assert(edgeFrequencyHz(352800u, 250u) == 1411200u);
    assert(edgeFrequencyHz(100u, 0u) == 0u);
    assert(nearFrequency(44100u, kWordSelectHz));
    assert(nearFrequency(1411200u, kBitClockHz));
    assert(!nearFrequency(0u, kWordSelectHz));
    assert(!nearFrequency(44100u, kBitClockHz));
    assert(!nearFrequency(1411200u, kWordSelectHz));
    assert(nearFrequency(40000u, kWordSelectHz));
    assert(!nearFrequency(30000u, kWordSelectHz));
}
