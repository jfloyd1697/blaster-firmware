#pragma once
#include <cstddef>
#include <cstdint>
constexpr int O_RDONLY = 0;
class File32 {
public:
    operator bool() const { return false; }
    int read(std::uint8_t*, std::size_t) { return 0; }
    bool available() const { return false; }
    std::uint32_t curPosition() const { return 0; }
    bool seekSet(std::uint32_t) { return false; }
    void close() {}
};
class StubSd {
public:
    int openCount = 0;
    File32 open(const char*, int) { ++openCount; return {}; }
};
