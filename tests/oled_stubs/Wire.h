#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class TwoWire {
public:
    std::vector<std::vector<std::uint8_t>> transmissions;
    std::vector<std::uint8_t> pending;
    bool connected = true;

    void beginTransmission(std::uint8_t) { pending.clear(); }
    std::size_t write(std::uint8_t value) { pending.push_back(value); return 1; }
    std::size_t write(const std::uint8_t* values, std::size_t size) {
        pending.insert(pending.end(), values, values + size);
        return size;
    }
    std::uint8_t endTransmission() {
        if (!connected) return 4;
        transmissions.push_back(pending);
        return 0;
    }
};
