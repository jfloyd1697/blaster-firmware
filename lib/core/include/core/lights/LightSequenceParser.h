#ifndef BLASTER_FIRMWARE_LIGHTSEQUENCEPARSER_H
#define BLASTER_FIRMWARE_LIGHTSEQUENCEPARSER_H

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/lights/ILights.h"

class ITextResourceLoader;

struct LightSequenceHeader {
    std::uint16_t ledCount = 0;
    std::uint32_t frameCount = 0;
    std::uint32_t totalDurationMs = 0;
};

// Binary format WFLSEQ01: header 20 bytes; frames are independently decodable.
// Per-frame reads bound peak memory to a single frame, not the entire animation.
class LightSequenceParser {
public:
    static bool readHeader(ITextResourceLoader& loader, const std::string& path, LightSequenceHeader& result);
    static bool readFrame(
        ITextResourceLoader& loader,
        const std::string& path,
        std::size_t offset,
        std::uint16_t ledCount,
        LightFrame& result,
        std::size_t& nextOffset);
    // Advance past a compiled frame using only its six-byte metadata.
    static bool skipFrame(
        ITextResourceLoader& loader, const std::string& path, std::size_t offset,
        std::uint16_t ledCount, std::size_t& nextOffset);
    static bool validateBinary(ITextResourceLoader& loader, const std::string& path);
    static bool readLayoutHeader(
        ITextResourceLoader& loader, const std::string& path, std::uint16_t& ledCount);
    static bool validateLayout(ITextResourceLoader& loader, const std::string& path);
    static constexpr std::size_t FirstFrameOffset = 20;
};

#endif // BLASTER_FIRMWARE_LIGHTSEQUENCEPARSER_H
