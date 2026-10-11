#include "core/weapons/WeaponLoader.h"

#include <filesystem>
#include <string>
#include <utility>

#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponsManifestParser.h"
#include "weapon_behavior/WeaponBehaviorParser.h"
#include "weapon_behavior/WeaponBehaviorValidation.h"

namespace {
std::string parentDir(const std::string& path) {
    return std::filesystem::path(path).parent_path().generic_string();
}

std::string joinPath(const std::string& base, const std::string& child) {
    return (std::filesystem::path(base) / child).generic_string();
}
}

std::vector<WeaponBank> WeaponLoader::loadBanks(
    ITextResourceLoader& loader,
    IDebug& debug,
    const std::string& manifestPath)
{
    debug.log("WeaponLoader: Loading weapon banks from manifest at " + manifestPath);
    const std::string manifestText = loader.loadText(manifestPath);
    if (manifestText.empty()) {
        debug.error("WeaponLoader: weapons manifest is missing or empty: " + manifestPath);
        return {};
    }

    WeaponsManifest manifest = WeaponsManifestParser::parseFromText(manifestText, &debug);
    if (manifest.banks.empty()) {
        debug.error("WeaponLoader: weapons manifest contains no valid banks");
        return {};
    }

    debug.log(
        "WeaponLoader: Successfully parsed weapons manifest with " +
        std::to_string(manifest.banks.size()) + " banks");

    const std::string manifestDir = parentDir(manifestPath);

    std::vector<WeaponBank> result;
    result.reserve(manifest.banks.size());

    for (const auto& bankManifest : manifest.banks) {
        WeaponBank bank;
        bank.name = bankManifest.name;

        debug.log("Parsing weapon bank " + bank.name);

        for (const auto& relativeBehaviorPath : bankManifest.weapons) {
            const std::string fullBehaviorPath = joinPath(manifestDir, relativeBehaviorPath);

            debug.log("Parsing weapon entry: " + fullBehaviorPath);

            WeaponEntry entry;
            entry.name = relativeBehaviorPath;
            entry.behaviorPath = fullBehaviorPath;

            bank.weapons.push_back(std::move(entry));
        }

        result.push_back(std::move(bank));
    }

    return result;
}
