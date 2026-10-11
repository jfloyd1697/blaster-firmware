#ifndef BLASTER_FIRMWARE_ASSET_LAYOUT_VALIDATOR_H
#define BLASTER_FIRMWARE_ASSET_LAYOUT_VALIDATOR_H

#pragma once

#include <cstddef>
#include <string>
#include <vector>

class IDebug;
class ITextResourceLoader;

struct AssetLayoutIssue {
    std::string path;
    std::string message;
};

enum class AssetValidationDepth {
    ReferencesOnly,
    FullContent,
};

struct AssetLayoutReport {
    bool ok = true;
    std::size_t bankCount = 0;
    std::size_t weaponCount = 0;
    std::size_t soundFileCount = 0;
    std::size_t lightSequenceCount = 0;
    std::vector<AssetLayoutIssue> issues;
};

class AssetLayoutValidator {
public:
    static constexpr const char* DefaultManifestPath = "assets/weapons/weapons_manifest.bin";

    static AssetLayoutReport validate(
        ITextResourceLoader& loader,
        IDebug* debug = nullptr,
        const std::string& manifestPath = DefaultManifestPath,
        AssetValidationDepth depth = AssetValidationDepth::FullContent);
};

#endif // BLASTER_FIRMWARE_ASSET_LAYOUT_VALIDATOR_H
