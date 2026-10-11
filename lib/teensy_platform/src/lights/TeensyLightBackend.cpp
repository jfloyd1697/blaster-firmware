#include "platform/teensy/lights/TeensyLightBackend.h"

#include <OctoWS2811.h>

#ifndef BLASTER_TEENSY_LED_COUNT
#define BLASTER_TEENSY_LED_COUNT 1
#endif

#ifndef BLASTER_TEENSY_LED_PIN
#define BLASTER_TEENSY_LED_PIN 2
#endif

static_assert(BLASTER_TEENSY_LED_COUNT > 0, "BLASTER_TEENSY_LED_COUNT must be greater than zero");

namespace {
constexpr int kBytesPerLed = 3;
constexpr int kLedPinCount = 1;
constexpr int kLedBufferWords = (BLASTER_TEENSY_LED_COUNT * kLedPinCount * kBytesPerLed + 3) / 4;
constexpr int kLedConfig = WS2811_GRB | WS2811_800kHz;

DMAMEM int displayMemory[kLedBufferWords];
int drawingMemory[kLedBufferWords];
byte ledPins[kLedPinCount] = {BLASTER_TEENSY_LED_PIN};

std::uint32_t packedColor(const LightColor& color) {
    return (static_cast<std::uint32_t>(color[0]) << 16u) |
           (static_cast<std::uint32_t>(color[1]) << 8u) |
           static_cast<std::uint32_t>(color[2]);
}
}

TeensyLightBackend::TeensyLightBackend() = default;

TeensyLightBackend::~TeensyLightBackend() {
    delete m_leds;
    m_leds = nullptr;
}

bool TeensyLightBackend::begin() {
    if (m_leds != nullptr) {
        return true;
    }
    m_leds = new OctoWS2811(
        BLASTER_TEENSY_LED_COUNT,
        displayMemory,
        drawingMemory,
        kLedConfig,
        kLedPinCount,
        ledPins);
    if (m_leds == nullptr) {
        return false;
    }
    m_leds->begin();
    return true;
}

std::size_t TeensyLightBackend::ledCount() const {
    return BLASTER_TEENSY_LED_COUNT;
}

void TeensyLightBackend::show(const std::vector<LightColor>& pixels) {
    if (m_leds == nullptr || pixels.size() != ledCount()) {
        return;
    }
    for (std::size_t index = 0; index < pixels.size(); ++index) {
        m_leds->setPixel(static_cast<int>(index), packedColor(pixels[index]));
    }
    m_leds->show();
}
