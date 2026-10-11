#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "core/assets/AssetLayoutValidator.h"
#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

namespace {
struct TestDebug final : IDebug {
    void log(const std::string& message) override {
        std::cout << "[LOG] " << message << '\n';
    }

    void error(const std::string& message) override {
        std::cerr << "[ERROR] " << message << '\n';
    }
};

class FileResourceLoader final : public ITextResourceLoader {
public:
    explicit FileResourceLoader(IDebug* debug) : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        const std::ifstream input(root + path, std::ios::binary);
        if (!input) {
            return {};
        }
        std::ostringstream stream;
        stream << input.rdbuf();
        return stream.str();
    }

    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        std::ifstream input(root + path, std::ios::binary);
        if (!input || !input.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string value(count, '\0');
        input.read(value.data(), static_cast<std::streamsize>(count));
        value.resize(static_cast<std::size_t>(input.gcount()));
        return value;
    }

    bool resourceExists(const std::string& path) const override {
        return std::filesystem::is_regular_file(root + path);
    }
private:
    const std::string root = "tests/data/compiled_assets/";
};
}

int main() {
    TestDebug debug;
    FileResourceLoader loader(&debug);
    const AssetLayoutReport report = AssetLayoutValidator::validate(
        loader,
        &debug,
        AssetLayoutValidator::DefaultManifestPath,
        AssetValidationDepth::FullContent);

    if (report.ok) {
        std::cout << "Asset layout valid: " << report.bankCount << " banks, "
                  << report.weaponCount << " weapons, " << report.soundFileCount
                  << " audio files, " << report.lightSequenceCount << " light sequences\n";
        return 0;
    }

    std::cerr << "Asset layout invalid: " << report.issues.size() << " issue(s)\n";
    for (const auto& issue : report.issues) {
        std::cerr << "  " << issue.path << ": " << issue.message << '\n';
    }
    return 1;
}
