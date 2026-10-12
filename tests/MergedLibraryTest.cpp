#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "core/assets/AssetLayoutValidator.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/weapons/WeaponsManifestParser.h"
#include "weapon_behavior/WeaponBehaviorParser.h"
#include "weapon_behavior/WeaponBehaviorValidation.h"

namespace {
class LocalLoader final : public ITextResourceLoader {
public:
    std::size_t largestRead = 0;

    std::string loadText(const std::string& relative) override {
        std::ifstream source(relative, std::ios::binary);
        return {std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()};
    }
    bool resourceExists(const std::string& relative) const override {
        return std::filesystem::is_regular_file(relative);
    }
    std::string readRange(const std::string& relative, std::size_t offset, std::size_t count) override {
        if (count > largestRead) largestRead = count;
        std::ifstream input(relative, std::ios::binary);
        if (!input || !input.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string data(count, '\0');
        input.read(data.data(), static_cast<std::streamsize>(count));
        data.resize(static_cast<std::size_t>(input.gcount()));
        return data;
    }
};
}

int main() {
    LocalLoader loader;
    const auto manifest = WeaponsManifestParser::parseFromText(
        loader.loadText(AssetLayoutValidator::DefaultManifestPath), nullptr);
    assert(manifest.banks.size() == 6);
    std::size_t weapons = 0;
    bool foundFarts = false;
    for (const auto& bank : manifest.banks) {
        if (bank.name == "Farts") {
            foundFarts = true;
            assert(bank.weapons.size() == 8);
        }
        for (const auto& entry : bank.weapons) {
            const std::string path = std::string("assets/weapons/") + entry;
            auto behavior = weapon_behavior::WeaponBehaviorParser::parse(loader.loadText(path), nullptr);
            const auto validation = weapon_behavior::validateWeaponBehavior(behavior);
            if (!validation.ok) {
                std::cerr << "Invalid weapon: " << path << "\n";
                for (const auto& issue : validation.errors) std::cerr << issue << "\n";
                return 1;
            }
            ++weapons;
        }
    }
    assert(foundFarts && weapons == 58);
    const auto report = AssetLayoutValidator::validate(
        loader, nullptr, AssetLayoutValidator::DefaultManifestPath,
        AssetValidationDepth::FullContent);
    if (!report.ok) {
        for (const auto& issue : report.issues) {
            std::cerr << issue.path << ": " << issue.message << "\n";
        }
        return 1;
    }
    assert(report.bankCount == 6);
    assert(report.weaponCount == 58);
    assert(report.lightSequenceCount > 0);
    assert(report.soundFileCount > 0);
    assert(loader.largestRead < 8192);
    std::cout << "Validated " << weapons << " weapons; " << report.soundFileCount
              << " clip references and " << report.lightSequenceCount << " light references\n";
}
