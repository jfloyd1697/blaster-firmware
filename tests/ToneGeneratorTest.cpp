#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "core/audio/ToneGenerator.h"

int main() {
    ToneGenerator tone;
    assert(!tone.configure(0, 44100, 6000));
    assert(!tone.configure(22050, 44100, 6000));
    assert(!tone.configure(440, 0, 6000));
    assert(!tone.configure(440, 44100, 0));
    assert(tone.configure(440, 44100, 6000));

    std::int16_t low = 32767;
    std::int16_t high = -32768;
    int positiveCrossings = 0;
    std::int16_t previous = tone.nextSample();
    for (int i = 1; i < 44100; ++i) {
        const std::int16_t sample = tone.nextSample();
        if (previous < 0 && sample >= 0) {
            ++positiveCrossings;
        }
        low = sample < low ? sample : low;
        high = sample > high ? sample : high;
        previous = sample;
    }
    assert(low >= -6000 && high <= 6000);
    assert(low < -5900 && high > 5900);
    assert(std::abs(positiveCrossings - 440) <= 1);

    std::cout << "ToneGeneratorTest passed: 440 Hz, 44100 frames, 16-bit PCM\n";
}
