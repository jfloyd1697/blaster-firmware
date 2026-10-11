#include "core/assets/AssetLayoutValidator.h"

#include <filesystem>

#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/lights/LightSequenceParser.h"
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

bool usesPortableSeparators(const std::string& path) {
    return path.find('\\') == std::string::npos;
}

void addIssue(AssetLayoutReport& report, const std::string& path, const std::string& message) {
    report.ok = false;
    report.issues.push_back({path, message});
}

void logProgress(IDebug* debug, const std::string& message) {
    if (debug) {
        debug->log("AssetLayoutValidator: " + message);
    }
}

void checkFile(
    const std::string& path,
    const char* kind,
    ITextResourceLoader& loader,
    AssetLayoutReport& report)
{
    if (!usesPortableSeparators(path)) {
        addIssue(report, path, std::string(kind) + " path must use '/' separators");
        return;
    }
    if (!loader.resourceExists(path)) {
        addIssue(report, path, std::string(kind) + " file does not exist");
    }
}

} // namespace

AssetLayoutReport AssetLayoutValidator::validate(
    ITextResourceLoader& loader,
    IDebug* debug,
    const std::string& manifestPath,
    const AssetValidationDepth depth)
{
    AssetLayoutReport report;
    logProgress(debug, "starting validation");
    logProgress(debug, "checking manifest path: " + manifestPath);

    if (!usesPortableSeparators(manifestPath)) {
        addIssue(report, manifestPath, "manifest path must use '/' separators");
        return report;
    }
    if (!loader.resourceExists(manifestPath)) {
        addIssue(report, manifestPath, "weapons manifest does not exist");
        return report;
    }

    logProgress(debug, "reading manifest");
    const std::string manifestText = loader.loadText(manifestPath);
    if (manifestText.empty()) {
        addIssue(report, manifestPath, "weapons manifest is empty or unreadable");
        return report;
    }

    logProgress(debug, "parsing manifest");
    const WeaponsManifest manifest = WeaponsManifestParser::parseFromText(manifestText, debug);
    if (manifest.banks.empty()) {
        addIssue(report, manifestPath, "weapons manifest contains no valid banks");
        return report;
    }

    report.bankCount = manifest.banks.size();
    logProgress(debug, "manifest parsed with " + std::to_string(report.bankCount) + " bank(s)");
    const std::string manifestDir = parentDir(manifestPath);

    for (const auto& bank : manifest.banks) {
        logProgress(
            debug,
            "validating bank '" + bank.name + "' with " + std::to_string(bank.weapons.size()) + " weapon(s)");

        for (const auto& relativeBehaviorPath : bank.weapons) {
            ++report.weaponCount;
            if (!usesPortableSeparators(relativeBehaviorPath)) {
                addIssue(report, relativeBehaviorPath, "weapon path must use '/' separators");
                continue;
            }

            const std::string behaviorPath = joinPath(manifestDir, relativeBehaviorPath);
            logProgress(debug, "validating weapon: " + behaviorPath);

            if (!loader.resourceExists(behaviorPath)) {
                addIssue(report, behaviorPath, "weapon runtime file does not exist");
                continue;
            }

            if (behaviorPath.size() < 4 || behaviorPath.substr(behaviorPath.size() - 4) != ".bin") {
                addIssue(report, behaviorPath, "embedded runtime requires weapon.bin (re-export SD assets)");
                continue;
            }
            const std::string behaviorText = loader.loadText(behaviorPath);
            if (!weapon_behavior::WeaponBehaviorParser::isBinaryFormat(behaviorText)) {
                addIssue(report, behaviorPath, "expected WFSMBIN1 binary weapon runtime");
                continue;
            }
            logProgress(debug, "reading weapon asset table: " + behaviorPath);
            const auto references = weapon_behavior::WeaponBehaviorParser::parseAssetReferences(behaviorText, debug);
            if (!references.valid) {
                addIssue(report, behaviorPath, "corrupt WFSMBIN1 asset tables");
                continue;
            }
            for (const auto& path : references.audioPaths) {
                ++report.soundFileCount;
                logProgress(debug, "checking audio asset: " + path);
                checkFile(path, "audio", loader, report);
            }
            for (const auto& path : references.lightSequencePaths) {
                ++report.lightSequenceCount;
                logProgress(debug, "checking light sequence asset: " + path);
                checkFile(path, "light sequence", loader, report);
                LightSequenceHeader sequenceHeader;
                if (loader.resourceExists(path) &&
                    (!LightSequenceParser::readHeader(loader, path, sequenceHeader) ||
                     (depth == AssetValidationDepth::FullContent &&
                      !LightSequenceParser::validateBinary(loader, path)))) {
                    addIssue(report, path, "invalid WFLSEQ01 light sequence");
                }
                // Compiled light layouts accompany each compiled sequence.
                const auto layoutPath = path.substr(0, path.size() - 4) + ".layout.bin";
                if (!loader.resourceExists(layoutPath)) {
                    addIssue(report, layoutPath, "compiled light layout missing");
                } else {
                    std::uint16_t layoutLedCount = 0;
                    const bool layoutOk = depth == AssetValidationDepth::ReferencesOnly
                        ? LightSequenceParser::readLayoutHeader(loader, layoutPath, layoutLedCount)
                        : LightSequenceParser::validateLayout(loader, layoutPath) &&
                          LightSequenceParser::readLayoutHeader(loader, layoutPath, layoutLedCount);
                    if (!layoutOk || layoutLedCount != sequenceHeader.ledCount) {
                        addIssue(report, layoutPath, "invalid WFLLAY01 layout or LED count differs from sequence");
                    }
                }
            }
            if (depth == AssetValidationDepth::FullContent) {
                logProgress(debug, "validating full weapon runtime: " + behaviorPath);
                const auto behavior = weapon_behavior::WeaponBehaviorParser::parse(behaviorText, debug);
                const auto validation = weapon_behavior::validateWeaponBehavior(behavior);
                for (const auto& issue : validation.errors) addIssue(report, behaviorPath, issue);
            }
            logProgress(debug, "finished weapon: " + behaviorPath);
        }
    }

    logProgress(
        debug,
        "validation complete with " + std::to_string(report.issues.size()) + " issue(s)");
    return report;
}
