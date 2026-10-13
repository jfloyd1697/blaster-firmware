#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bridge {

constexpr std::uint8_t kSyncFirst = 0xA5;
constexpr std::uint8_t kSyncSecond = 0x5A;
constexpr std::size_t kMaximumPayload = 512;
constexpr std::size_t kFrameOverhead = 9;

enum class Op : std::uint8_t {
    Hello = 1, SetMode = 2, Heartbeat = 3, Input = 4,
    PlayAudio = 5, StopAudio = 6, PlayLight = 7, StopLight = 8, LedFrame = 9,
    FileBegin = 10, FileChunk = 11, FileCommit = 12, Status = 13, Activate = 14,
    FileList = 15, FileStat = 16, FileRead = 17, FileDelete = 18,
    FileMkdir = 19, FileRename = 20, FileAbort = 21, FileHash = 22, FileRelease = 23,
    Ack = 128, Event = 129,
};

enum class Result : std::uint8_t {
    Ok = 0, Invalid = 1, NotFound = 2, IoError = 3,
    Busy = 4, InvalidMode = 5, ChecksumMismatch = 6,
};

inline std::uint16_t crc16(const std::uint8_t* data, const std::size_t length) {
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < length; ++i) {
        crc ^= static_cast<std::uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = static_cast<std::uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
        }
    }
    return crc;
}

inline std::uint32_t crc32Update(std::uint32_t crc, const std::uint8_t* data, const std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
        }
    }
    return crc;
}

inline std::uint16_t read16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0] | (static_cast<std::uint16_t>(data[1]) << 8));
}
inline std::uint32_t read32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) |
        (static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24);
}
inline void write16(std::uint8_t* data, const std::uint16_t value) {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8);
}
inline void write32(std::uint8_t* data, const std::uint32_t value) {
    for (int i = 0; i < 4; ++i) data[i] = static_cast<std::uint8_t>(value >> (8 * i));
}

struct Frame {
    Op op = Op::Hello;
    std::uint16_t sequence = 0;
    std::uint16_t size = 0;
    std::array<std::uint8_t, kMaximumPayload> data{};
};

// Fixed-storage incremental decoder: no dynamic allocation or SD writes on input parsing.
class Parser {
public:
    bool push(const std::uint8_t byte, Frame& complete) {
        if (m_used == 0) {
            if (byte == kSyncFirst) m_buffer[m_used++] = byte;
            return false;
        }
        if (m_used == 1) {
            if (byte == kSyncSecond) m_buffer[m_used++] = byte;
            else m_used = byte == kSyncFirst ? 1 : 0;
            return false;
        }
        m_buffer[m_used++] = byte;
        if (m_used == 4 && read16(m_buffer.data() + 2) > kMaximumPayload) {
            reset(byte);
            return false;
        }
        if (m_used < 4) return false;
        const std::size_t expected = read16(m_buffer.data() + 2) + kFrameOverhead;
        if (m_used < expected) return false;
        const bool valid = crc16(m_buffer.data() + 2, expected - 4) == read16(m_buffer.data() + expected - 2);
        if (valid) {
            complete.op = static_cast<Op>(m_buffer[4]);
            complete.sequence = read16(m_buffer.data() + 5);
            complete.size = read16(m_buffer.data() + 2);
            for (std::size_t i = 0; i < complete.size; ++i) complete.data[i] = m_buffer[7 + i];
        }
        reset(byte);
        return valid;
    }

private:
    void reset(const std::uint8_t lastByte) {
        m_used = lastByte == kSyncFirst ? 1 : 0;
        if (m_used) m_buffer[0] = kSyncFirst;
    }
    std::array<std::uint8_t, kMaximumPayload + kFrameOverhead> m_buffer{};
    std::size_t m_used = 0;
};

} // namespace bridge
