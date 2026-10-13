#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "core/assets/AssetLayoutValidator.h"
#include "core/lights/ILightBackend.h"
#include "core/lights/LightEngine.h"
#include "core/lights/LightSequenceParser.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/time/ITime.h"
#include "core/weapons/WeaponsManifestParser.h"

namespace {
const std::string kFixtureRoot = "tests/data/compiled_assets/";

class TestLoader final : public ITextResourceLoader {
public:
    std::size_t maxRequested = 0;
    std::string loadText(const std::string& path) override {
        std::ifstream input(kFixtureRoot + path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }
    bool resourceExists(const std::string& path) const override {
        return std::filesystem::is_regular_file(kFixtureRoot + path);
    }
    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        if (count > maxRequested) maxRequested = count;
        std::ifstream input(kFixtureRoot + path, std::ios::binary);
        if (!input || !input.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string output(count, '\0');
        input.read(output.data(), static_cast<std::streamsize>(count));
        output.resize(static_cast<std::size_t>(input.gcount()));
        return output;
    }
};

class TestTime final : public ITime {
public:
    [[nodiscard]] unsigned long millis() const override { return time; }
    unsigned long time = 0;
};

class TestBackend final : public ILightBackend {
public:
    bool begin() override { return true; }
    std::size_t ledCount() const override { return 2; }
    void show(const std::vector<LightColor>& value) override { pixels = value; }
    std::vector<LightColor> pixels;
};
}

int main() {
    TestLoader loader;
    const auto manifest = WeaponsManifestParser::parseFromText(
        loader.loadText("assets/weapons/weapons_manifest.bin"), nullptr);
    assert(manifest.banks.size() == 1);
    assert(manifest.banks.front().weapons == std::vector<std::string>({"Default/test/weapon.bin"}));
    assert(WeaponsManifestParser::parseFromText("{\"banks\":[]}", nullptr).banks.empty());
    assert(WeaponsManifestParser::parseFromText("WFMAN001", nullptr).banks.empty());

    const auto referencesOnly = AssetLayoutValidator::validate(
        loader, nullptr, AssetLayoutValidator::DefaultManifestPath, AssetValidationDepth::ReferencesOnly);
    assert(referencesOnly.ok);
    assert(referencesOnly.bankCount == 1);
    assert(referencesOnly.weaponCount == 1);
    assert(referencesOnly.soundFileCount == 1);
    assert(referencesOnly.lightSequenceCount == 1);

    const auto full = AssetLayoutValidator::validate(
        loader, nullptr, AssetLayoutValidator::DefaultManifestPath, AssetValidationDepth::FullContent);
    assert(full.ok);
    assert(LightSequenceParser::validateBinary(loader, "assets/lights/flash.bin"));
    assert(LightSequenceParser::validateLayout(loader, "assets/lights/flash.layout.bin"));
    assert(!LightSequenceParser::validateBinary(loader, "assets/audio/fire.wav"));
    assert(!LightSequenceParser::validateLayout(loader, "assets/lights/flash.bin"));

    TestTime time;
    auto backend = std::make_unique<TestBackend>();
    auto* rawBackend = backend.get();
    LightEngine engine(std::move(backend), &time);
    assert(engine.begin());
    assert(engine.playSequenceFile("assets/lights/flash.bin", loader, LightSequencePlayback{true}));
    engine.update();
    assert((rawBackend->pixels[1] == LightColor{0, 255, 0}));
    time.time = 99;
    engine.update();
    assert((rawBackend->pixels[1] == LightColor{0, 255, 0}));
    time.time = 100;
    engine.update();
    assert((rawBackend->pixels[1] == LightColor{0, 0, 0}));
    assert(rawBackend->pixels[0][0] >= 127 && rawBackend->pixels[0][0] <= 129);
    time.time = 300;
    engine.update();
    assert((rawBackend->pixels[1] == LightColor{0, 255, 0}));
    time.time = 450;
    engine.update();
    assert(rawBackend->pixels[0][0] >= 127 && rawBackend->pixels[0][0] <= 129);
    // The binary light path must never request the entire sequence at once.
    assert(loader.maxRequested <= 4096 * 6);

    // This animation is ~180 KiB. The player still retains only one frame.
    std::size_t largeSize = std::filesystem::file_size(kFixtureRoot + "assets/lights/large.bin");
    assert(largeSize > 180000);
    loader.maxRequested = 0;
    assert(engine.playSequenceFile("assets/lights/large.bin", loader, LightSequencePlayback{true}));
    time.time = 450 + 33 * 499;
    engine.update();
    assert(loader.maxRequested < 1024);
    time.time = 450 + 33 * 500;
    engine.update();
    assert(loader.maxRequested < 1024);

    engine.stopSequence();
    assert(!engine.playSequenceFile("assets/audio/fire.wav", loader, LightSequencePlayback{}));
    return 0;
}
