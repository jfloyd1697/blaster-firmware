#include "core/lights/LightSequenceParser.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "core/text_resource_loader/ITextResourceLoader.h"

namespace {
constexpr std::uint16_t kMaxLeds = 4096;
constexpr std::uint32_t kMaxFrames = 1000000;

std::uint16_t u16(const std::string& data, std::size_t pos) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint8_t>(data[pos]) | (static_cast<unsigned>(static_cast<std::uint8_t>(data[pos + 1])) << 8));
}

std::uint32_t u32(const std::string& data, std::size_t pos) {
    return static_cast<std::uint32_t>(u16(data, pos)) |
        (static_cast<std::uint32_t>(u16(data, pos + 2)) << 16);
}
}

bool LightSequenceParser::readHeader(
    ITextResourceLoader& loader, const std::string& path, LightSequenceHeader& result)
{
    const auto data = loader.readRange(path, 0, FirstFrameOffset);
    if (data.size() != FirstFrameOffset || data.compare(0, 8, "WFLSEQ01", 8) != 0 || u16(data, 8) != 1) {
        return false;
    }
    result.ledCount = u16(data, 10);
    result.frameCount = u32(data, 12);
    result.totalDurationMs = u32(data, 16);
    return result.ledCount > 0 && result.ledCount <= kMaxLeds &&
        result.frameCount > 0 && result.frameCount <= kMaxFrames && result.totalDurationMs > 0;
}

bool LightSequenceParser::readFrame(
    ITextResourceLoader& loader, const std::string& path, std::size_t offset,
    std::uint16_t ledCount, LightFrame& result, std::size_t& nextOffset)
{
    // A small sequence frame can be read with one SD transaction. The reader
    // still derives the next frame offset from the encoded pixel count, so
    // reading bytes from a following frame does not affect playback.
    constexpr std::size_t kSingleReadLedLimit = 256;
    const bool singleRead = ledCount <= kSingleReadLedLimit;
    const std::string data = loader.readRange(
        path, offset, 6 + (singleRead ? static_cast<std::size_t>(ledCount) * 6 : 0));
    if (data.size() < 6) return false;
    const auto duration = u32(data, 0);
    const auto pixelCount = u16(data, 4);
    if (duration == 0 || pixelCount > ledCount || pixelCount > kMaxLeds ||
        offset > std::numeric_limits<std::size_t>::max() - 6 - static_cast<std::size_t>(pixelCount) * 6) {
        return false;
    }
    const std::size_t payloadSize = static_cast<std::size_t>(pixelCount) * 6;
    std::string separatePayload;
    const std::string* pixels = &data;
    std::size_t payloadOffset = 6;
    if (!singleRead) {
        separatePayload = loader.readRange(path, offset + 6, payloadSize);
        pixels = &separatePayload;
        payloadOffset = 0;
    }
    if (pixels->size() < payloadOffset + payloadSize) return false;

    LightFrame frame;
    frame.durationMs = duration;
    frame.pixels.reserve(pixelCount);
    for (std::size_t i = 0; i < pixelCount; ++i) {
        const std::size_t pos = payloadOffset + i * 6;
        const auto index = u16(*pixels, pos);
        if (index >= ledCount) return false;
        LightPixel pixel;
        pixel.index = index;
        pixel.color = {
            static_cast<std::uint8_t>((*pixels)[pos + 2]),
            static_cast<std::uint8_t>((*pixels)[pos + 3]),
            static_cast<std::uint8_t>((*pixels)[pos + 4]),
        };
        pixel.intensity = static_cast<std::uint8_t>((*pixels)[pos + 5]) / 255.0F;
        frame.pixels.push_back(std::move(pixel));
    }
    nextOffset = offset + 6 + payloadSize;
    result = std::move(frame);
    return true;
}

bool LightSequenceParser::validateBinary(ITextResourceLoader& loader, const std::string& path) {
    LightSequenceHeader header;
    if (!readHeader(loader, path, header)) return false;
    // Validate the frame index and duration without retaining the full sequence.
    std::size_t offset = FirstFrameOffset;
    std::uint64_t duration = 0;
    for (std::uint32_t i = 0; i < header.frameCount; ++i) {
        LightFrame frame;
        std::size_t next = 0;
        if (!readFrame(loader, path, offset, header.ledCount, frame, next)) return false;
        duration += frame.durationMs;
        offset = next;
    }
    return duration == header.totalDurationMs && loader.readRange(path, offset, 1).empty();
}

bool LightSequenceParser::readLayoutHeader(
    ITextResourceLoader& loader, const std::string& path, std::uint16_t& ledCount)
{
    const auto header = loader.readRange(path, 0, 12);
    if (header.size() != 12 || header.compare(0, 8, "WFLLAY01", 8) != 0 || u16(header, 8) != 1) {
        return false;
    }
    ledCount = u16(header, 10);
    return ledCount > 0 && ledCount <= kMaxLeds;
}

bool LightSequenceParser::validateLayout(ITextResourceLoader& loader, const std::string& path) {
    std::uint16_t count = 0;
    if (!readLayoutHeader(loader, path, count)) return false;
    constexpr std::size_t kChunkLeds = 32;
    for (std::size_t first = 0; first < count; first += kChunkLeds) {
        const auto number = std::min(kChunkLeds, static_cast<std::size_t>(count) - first);
        const auto entries = loader.readRange(path, 12 + first * 14, number * 14);
        if (entries.size() != number * 14) return false;
        for (std::size_t i = 0; i < number; ++i) {
            if (u16(entries, i * 14) != first + i) return false;
        }
    }
    return loader.readRange(path, 12 + static_cast<std::size_t>(count) * 14, 1).empty();
}
