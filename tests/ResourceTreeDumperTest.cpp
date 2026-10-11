#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/debug/IDebug.h"
#include "core/storage/ResourceTreeDumper.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

namespace {
class CapturingDebug final : public IDebug {
public:
    void log(const std::string& message) override {
        logs.push_back(message);
    }

    void error(const std::string& message) override {
        errors.push_back(message);
    }

    std::vector<std::string> logs;
    std::vector<std::string> errors;
};

class FakeResourceLoader final : public ITextResourceLoader {
public:
    FakeResourceLoader() = default;

    std::string loadText(const std::string&) override {
        return {};
    }

    bool resourceExists(const std::string&) const override {
        return false;
    }

    ResourceDirectoryListing listDirectory(
        const std::string& path,
        std::size_t maxEntries) const override {
        const auto found = directories.find(path);
        if (found == directories.end()) {
            return {};
        }

        ResourceDirectoryListing result;
        result.ok = true;
        result.entries = found->second;
        if (result.entries.size() > maxEntries) {
            result.entries.resize(maxEntries);
            result.truncated = true;
        }
        return result;
    }

    std::map<std::string, std::vector<ResourceEntry>> directories;
};
}

int main() {
    FakeResourceLoader loader;
    loader.directories["/"] = {
        ResourceEntry{.name = "assets", .isDirectory = true},
        ResourceEntry{.name = "readme.txt", .isDirectory = false, .size = 10},
    };
    loader.directories["/assets"] = {
        ResourceEntry{.name = "audio", .isDirectory = true},
        ResourceEntry{.name = "weapons", .isDirectory = true},
    };
    loader.directories["/assets/audio"] = {
        ResourceEntry{.name = "shot.wav", .isDirectory = false, .size = 100},
    };
    loader.directories["/assets/weapons"] = {
        ResourceEntry{.name = "weapons_manifest.json", .isDirectory = false, .size = 200},
    };

    CapturingDebug debug;
    ResourceTreeDumpOptions options;
    options.maxDepth = 3;
    options.maxEntriesPerDirectory = 16;
    ResourceTreeDumper::dump(loader, debug, "/", options);

    assert(debug.errors.empty());
    assert(debug.logs.size() == 9);
    assert(debug.logs[0] == "Storage structure:");
    assert(debug.logs[1] == "[DIR]  /");
    assert(debug.logs[2] == "  [DIR]  assets");
    assert(debug.logs[3] == "    [DIR]  audio");
    assert(debug.logs[4] == "      [FILE] shot.wav");
    assert(debug.logs[5] == "    [DIR]  weapons");
    assert(debug.logs[6] == "      [FILE] weapons_manifest.json");
    assert(debug.logs[7] == "  [FILE] readme.txt");
    assert(debug.logs[8] == "End storage structure");
    return 0;
}
