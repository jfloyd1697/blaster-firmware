//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H

#pragma once

#include "core/text_resource_loader/ITextResourceLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

class PCTextResourceLoader : public ITextResourceLoader {
public:
    explicit PCTextResourceLoader(IDebug* debug) : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        if (!resourceExists(path)) {
            if (m_debug != nullptr) {
                m_debug->error("PCTextResourceLoader: file not found: " + path);
            }
            return {};
        }

        const std::ifstream in(normalizePath(path), std::ios::in | std::ios::binary);
        if (!in) {
            if (m_debug != nullptr) {
                m_debug->error("PCTextResourceLoader: failed to open: " + path);
            }
            return {};
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        std::ifstream file(normalizePath(path), std::ios::binary);
        if (!file || !file.seekg(static_cast<std::streamoff>(offset))) return {};
        std::string result(count, '\0');
        file.read(result.data(), static_cast<std::streamsize>(count));
        result.resize(static_cast<std::size_t>(file.gcount()));
        return result;
    }

    bool resourceExists(const std::string& path) const override {
        return std::filesystem::is_regular_file(normalizePath(path));
    }

    ResourceDirectoryListing listDirectory(
        const std::string& path,
        std::size_t maxEntries) const override {
        ResourceDirectoryListing result;
        const std::filesystem::path directoryPath = normalizePath(path);

        std::error_code ec;
        if (!std::filesystem::is_directory(directoryPath, ec) || ec) {
            return result;
        }

        result.ok = true;
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath, ec)) {
            if (ec) {
                result.ok = false;
                result.entries.clear();
                return result;
            }

            if (result.entries.size() >= maxEntries) {
                result.truncated = true;
                break;
            }

            ResourceEntry resourceEntry;
            resourceEntry.name = entry.path().filename().string();
            resourceEntry.isDirectory = entry.is_directory(ec);
            if (ec) {
                ec.clear();
                resourceEntry.isDirectory = false;
            }
            if (!resourceEntry.isDirectory && entry.is_regular_file(ec) && !ec) {
                resourceEntry.size = static_cast<std::size_t>(entry.file_size(ec));
                if (ec) {
                    resourceEntry.size = 0;
                    ec.clear();
                }
            }
            result.entries.push_back(std::move(resourceEntry));
        }

        std::sort(
            result.entries.begin(),
            result.entries.end(),
            [](const ResourceEntry& left, const ResourceEntry& right) {
                if (left.isDirectory != right.isDirectory) {
                    return left.isDirectory > right.isDirectory;
                }
                return left.name < right.name;
            });
        return result;
    }

private:
    static std::filesystem::path normalizePath(const std::string& path) {
        const std::filesystem::path sdRoot = std::filesystem::current_path() / "sdcard";
        if (path.empty() || path == "/") {
            return sdRoot;
        }
        const std::filesystem::path requested(path);
        return sdRoot / requested.relative_path();
    }
};

#endif // BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H
