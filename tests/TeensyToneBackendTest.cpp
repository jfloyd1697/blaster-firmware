#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <Arduino.h>
#include <Audio.h>

#include "core/debug/IDebug.h"
#include "platform/teensy/audio/TeensyAudioBackend.h"

namespace {
unsigned long testClockMs = 0;
struct TestDebug : IDebug {
    void log(const std::string&) override {}
    void error(const std::string& message) override { errors.push_back(message); }
    std::vector<std::string> errors;
};
}
unsigned long millis() { return testClockMs; }
void delay(unsigned long ms) { testClockMs += ms; }

int main() {
    TestDebug debug;
    TeensyAudioBackend audio(&debug);
    assert(mixerConnectionCount == 6);
    assert(audio.begin());
    assert(audio.playTone(660, 350));
    assert(lastToneFrequency == 660.0F);
    assert(lastToneAmplitude > 0.0F);
    testClockMs = 349;
    audio.update();
    assert(lastToneAmplitude > 0.0F);
    testClockMs = 350;
    audio.update();
    assert(lastToneAmplitude == 0.0F);
    assert(!audio.playTone(0, 350));
    assert(audio.playTone(880, 350));
    audio.stop();
    assert(lastToneAmplitude == 0.0F);
    assert(debug.errors.empty());
}
