//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_ESPTEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_ESPTEXTRESOURCELOADER_H

#pragma once

#include "core/text_resource_loader/ITextResourceLoader.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <string>
#include <utility>

#include "core/debug/IDebug.h"

class EspSdTextResourceLoader : public ITextResourceLoader {
public:
    explicit EspSdTextResourceLoader(IDebug* debug) : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file || file.isDirectory()) {
            if (m_debug != nullptr) {
                m_debug->error("EspSdTextResourceLoader: failed to open: " + path);
            }
            if (file) {
                file.close();
            }
            return {};
        }

        String content;
        while (file.available()) {
            content += static_cast<char>(file.read());
        }
        file.close();
        return std::string(content.c_str());
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

#endif // BLASTER_FIRMWARE_ESPTEXTRESOURCELOADER_H
