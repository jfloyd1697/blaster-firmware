#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "../src/BridgeProtocol.h"

int main() {
    assert(bridge::crc16(reinterpret_cast<const std::uint8_t*>("123456789"), 9) == 0x29B1);
    assert(~bridge::crc32Update(0xFFFFFFFFu,
        reinterpret_cast<const std::uint8_t*>("123456789"), 9) == 0xCBF43926u);

    const std::uint8_t payload[] = {1, 2, 3, 4};
    std::vector<std::uint8_t> message = {
        bridge::kSyncFirst, bridge::kSyncSecond, 4, 0,
        static_cast<std::uint8_t>(bridge::Op::FileChunk), 0x34, 0x12,
        1, 2, 3, 4, 0, 0,
    };
    bridge::write16(message.data() + message.size() - 2,
                    bridge::crc16(message.data() + 2, message.size() - 4));
    bridge::Parser parser;
    bridge::Frame frame;
    for (std::size_t i = 0; i < message.size() - 1; ++i) assert(!parser.push(message[i], frame));
    assert(parser.push(message.back(), frame));
    assert(frame.op == bridge::Op::FileChunk && frame.sequence == 0x1234 && frame.size == 4);
    assert(std::memcmp(frame.data.data(), payload, 4) == 0);
    message.back() ^= 0x20;
    for (auto byte : message) assert(!parser.push(byte, frame));
    // Ignore stray console bytes and recover at the next synchronization marker.
    for (auto byte : std::array<std::uint8_t, 4>{'L', 'O', 'G', '\n'})
        assert(!parser.push(byte, frame));
    message.back() ^= 0x20;
    bool accepted = false;
    for (auto byte : message) accepted = parser.push(byte, frame) || accepted;
    assert(accepted);
}
