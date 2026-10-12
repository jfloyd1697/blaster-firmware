#pragma once

#include <cstdint>

namespace stm32_i2s_loopback {
constexpr std::uint32_t kSampleRateHz = 44100u;
constexpr std::uint32_t kWordSelectHz = kSampleRateHz;
constexpr std::uint32_t kBitClockHz = kSampleRateHz * 16u * 2u;
inline bool running = false;
inline std::uint32_t suppliedEdges = 0u;
inline int starts = 0;
inline int stops = 0;
inline bool begin() {
    running = true;
    ++starts;
    return true;
}
inline void resetCounter() { suppliedEdges = 0u; }
inline std::uint32_t runReferencePulseTest() { return 250u; }
inline std::uint32_t takeRisingEdges() {
    const auto edges = suppliedEdges;
    suppliedEdges = 0u;
    return edges;
}
inline void end() {
    running = false;
    ++stops;
}
inline bool nearFrequency(const std::uint32_t measuredHz, const std::uint32_t expectedHz) {
    const std::uint64_t difference = measuredHz > expectedHz
        ? static_cast<std::uint64_t>(measuredHz - expectedHz)
        : static_cast<std::uint64_t>(expectedHz - measuredHz);
    return difference * 100u <= static_cast<std::uint64_t>(expectedHz) * 15u;
}
inline std::uint32_t edgeFrequencyHz(const std::uint32_t edges, const std::uint32_t elapsedMs) {
    return elapsedMs == 0u ? 0u : static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(edges) * 1000u + elapsedMs / 2u) / elapsedMs);
}
}
