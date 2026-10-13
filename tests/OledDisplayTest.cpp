#include <cassert>
#include <cstdint>
#include <vector>

#include "display/Ssd1306StatusDisplay.h"

namespace {
std::vector<std::uint8_t> collectPixels(const TwoWire& wire, std::size_t startIndex) {
    std::vector<std::uint8_t> pixels;
    for (std::size_t i = startIndex; i < wire.transmissions.size(); ++i) {
        const auto& packet = wire.transmissions[i];
        if (packet.empty() || packet.front() != 0x40) continue;
        assert(packet.size() <= 9); // Control byte plus at most eight pixels.
        pixels.insert(pixels.end(), packet.begin() + 1, packet.end());
    }
    return pixels;
}
}

int main() {
    TwoWire wire;
    Ssd1306StatusDisplay display(wire);
    assert(display.begin());

    DisplayStatus status;
    status.bankName = "Blaster";
    status.bankCount = 6;
    status.bankIndex = 1;
    status.weaponName = "LEGO STAR WARS E-11 BLASTER";
    status.weaponCount = 12;
    status.weaponIndex = 3;
    status.behaviorState = "idle";
    status.weaponState = DisplayWeaponState::Ready;
    display.present(status);
    const std::size_t firstPacket = wire.transmissions.size();
    for (int i = 0; i < 100; ++i) display.service(100);
    const auto firstPixels = collectPixels(wire, firstPacket);
    assert(firstPixels.size() == 128 * 4);
    assert(firstPixels[0] != 0); // A bank label was rendered.

    status.progress = DisplayProgress{10, 20};
    display.present(status);
    display.setControlMode(DisplayControlMode::RemoteInput);
    const std::size_t secondPacket = wire.transmissions.size();
    for (int i = 0; i < 100; ++i) display.service(200);
    const auto secondPixels = collectPixels(wire, secondPacket);
    assert(secondPixels.size() == firstPixels.size());
    assert(secondPixels != firstPixels);

    // Scroll a long weapon name without touching the display at every control tick.
    const std::size_t scrollPacket = wire.transmissions.size();
    display.service(700);
    for (int i = 0; i < 100; ++i) display.service(700);
    const auto scrollPixels = collectPixels(wire, scrollPacket);
    assert(scrollPixels.size() == firstPixels.size());
    assert(scrollPixels != secondPixels);

    // A removed I2C peripheral must not be retried during real-time operation.
    wire.connected = false;
    display.setControlMode(DisplayControlMode::DirectControl);
    display.service(800);
    const std::size_t count = wire.transmissions.size();
    for (int i = 0; i < 100; ++i) display.service(800);
    assert(wire.transmissions.size() == count);

    TwoWire disconnected;
    disconnected.connected = false;
    Ssd1306StatusDisplay missing(disconnected);
    assert(!missing.begin());
    return 0;
}
