#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "core/lights/ILightBackend.h"
#include "core/lights/LightEngine.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/time/ITime.h"

namespace {
std::size_t allocations = 0;
}

void* operator new(const std::size_t bytes) {
    if (void* memory = std::malloc(bytes == 0 ? 1 : bytes)) {
        ++allocations;
        return memory;
    }
    throw std::bad_alloc();
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
void push16(std::string& bytes, const unsigned value) {
    bytes.push_back(static_cast<char>(value & 0xffu));
    bytes.push_back(static_cast<char>((value >> 8u) & 0xffu));
}

void push32(std::string& bytes, const unsigned value) {
    push16(bytes, value & 0xffffu);
    push16(bytes, value >> 16u);
}

class MemoryLoader final : public ITextResourceLoader {
public:
    MemoryLoader() {
        sequence = "WFLSEQ01";
        push16(sequence, 1);
        push16(sequence, 61);
        push32(sequence, 2);
        push32(sequence, 100);
        for (int frame = 0; frame < 2; ++frame) {
            push32(sequence, 50);
            push16(sequence, 61);
            for (int led = 0; led < 61; ++led) {
                push16(sequence, static_cast<unsigned>(led));
                sequence.push_back(static_cast<char>(frame == 0 ? 50 : 150));
                sequence.push_back(0);
                sequence.push_back(0);
                sequence.push_back(static_cast<char>(255));
            }
        }
    }

    std::string loadText(const std::string&) override { return sequence; }
    bool resourceExists(const std::string&) const override { return true; }
    std::string readRange(const std::string&, std::size_t offset, std::size_t count) override {
        ++rangeReads;
        if (offset >= sequence.size()) return {};
        return sequence.substr(offset, count);
    }
    std::string sequence;
    int rangeReads = 0;
};

class Clock final : public ITime {
public:
    unsigned long millis() const override { return ms; }
    unsigned long ms = 0;
};

class Backend final : public ILightBackend {
public:
    bool begin() override { return true; }
    std::size_t ledCount() const override { return 61; }
    void show(const std::vector<LightColor>& pixels) override {
        ++shows;
        firstRed = pixels[0][0];
    }
    unsigned shows = 0;
    unsigned firstRed = 0;
};
}

int main() {
    MemoryLoader loader;
    Clock clock;
    auto backend = std::make_unique<Backend>();
    auto* hardware = backend.get();
    LightEngine lights(std::move(backend), &clock);
    assert(lights.begin());
    assert(lights.playSequenceFile("ring.bin", loader, LightSequencePlayback{true}));
    assert(loader.rangeReads == 2); // binary header + first frame

    lights.update();
    assert(hardware->firstRed == 50);
    const unsigned displays = hardware->shows;
    const int reads = loader.rangeReads;
    const std::size_t before = allocations;
    for (int repeat = 0; repeat < 2000; ++repeat) {
        clock.ms = 25;
        lights.update();
    }
    assert(allocations == before); // no new per-tick frame buffer
    assert(hardware->shows == displays);
    assert(loader.rangeReads == reads);

    clock.ms = 50;
    lights.update();
    assert(hardware->firstRed == 50); // deadline miss does not trigger SD access
    assert(loader.rangeReads == reads);
    assert(lights.missedPrefetchCount() > 0);
    lights.prefetchNextFrame();
    assert(loader.rangeReads == reads + 1);
    lights.update();
    assert(hardware->firstRed == 150); // cached frame advances without SD access

    // The previous frame must be restored after an overriding effect ends.
    weapon_behavior::LightPatternDef flash;
    flash.mode = weapon_behavior::LightPatternMode::Solid;
    flash.color = {255, 0, 0};
    flash.brightness = 255;
    flash.durationMs = 10;
    lights.flashPattern(flash);
    lights.update();
    assert(hardware->firstRed == 255);
    clock.ms = 62;
    lights.update();
    assert(hardware->firstRed == 150);

    clock.ms = 100;
    lights.update();
    assert(hardware->firstRed == 50);
    assert(loader.rangeReads == reads + 1); // loop reuses cached first frame
    return 0;
}
