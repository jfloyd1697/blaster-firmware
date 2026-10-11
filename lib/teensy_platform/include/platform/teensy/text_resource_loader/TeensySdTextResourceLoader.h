#ifndef BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H
#define BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H

#pragma once

#include <SD.h>

#include <string>
#include <utility>

#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

class TeensySdTextResourceLoader : public ITextResourceLoader {
public:
    explicit TeensySdTextResourceLoader(IDebug* debug)
        : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file || file.isDirectory()) {
            if (m_debug != nullptr) {
                m_debug->error("TeensySdTextResourceLoader: failed to open: " + path);
            }
            if (file) {
                file.close();
            }
            return {};
        }

        std::string content;
        content.reserve(file.size());
        while (file.available()) {
            content.push_back(static_cast<char>(file.read()));
        }
        file.close();
        return content;
    }

    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file || file.isDirectory() || offset > file.size() || !file.seek(offset)) {
            if (file) file.close();
            return {};
        }
        std::string result;
        const auto available = static_cast<std::size_t>(file.size()) - offset;
        result.reserve(count < available ? count : available);
        for (std::size_t i = 0; i < count && file.available(); ++i) {
            const int value = file.read();
            if (value < 0) break;
            result.push_back(static_cast<char>(value));
        }
        file.close();
        return result;
    }

    bool resourceExists(const std::string& path) const override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file) {
            return false;
        }
        const bool exists = !file.isDirectory();
        file.close();
        return exists;
    }

    ResourceDirectoryListing listDirectory(
        const std::string& path,
        std::size_t maxEntries) const override {
        ResourceDirectoryListing result;
        File directory = SD.open(path.c_str(), FILE_READ);
        if (!directory || !directory.isDirectory()) {
            if (directory) {
                directory.close();
            }
            return result;
        }

        result.ok = true;
        File entry = directory.openNextFile();
        while (entry) {
            if (result.entries.size() >= maxEntries) {
                result.truncated = true;
                entry.close();
                break;
            }

            ResourceEntry resourceEntry;
            resourceEntry.name = baseName(entry.name());
            resourceEntry.isDirectory = entry.isDirectory();
            if (!resourceEntry.isDirectory) {
                resourceEntry.size = static_cast<std::size_t>(entry.size());
            }
            result.entries.push_back(std::move(resourceEntry));

            entry.close();
            entry = directory.openNextFile();
        }
        directory.close();
        return result;
    }

private:
    static std::string baseName(const char* path) {
        std::string name = path == nullptr ? "" : path;
        const std::size_t separator = name.find_last_of('/');
        if (separator != std::string::npos) {
            return name.substr(separator + 1);
        }
        return name;
    }
};

#endif // BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H
