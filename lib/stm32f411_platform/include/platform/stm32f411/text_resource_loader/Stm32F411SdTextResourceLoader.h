#ifndef BLASTER_STM32F411_SD_TEXT_RESOURCE_LOADER_H
#define BLASTER_STM32F411_SD_TEXT_RESOURCE_LOADER_H

#pragma once

#include <string>
#include <utility>

#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

class Stm32F411SdTextResourceLoader : public ITextResourceLoader {
public:
    explicit Stm32F411SdTextResourceLoader(IDebug* debug)
        : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!file || file.isDir()) {
            if (m_debug != nullptr) {
                m_debug->error("Stm32F411SdTextResourceLoader: failed to open: " + path);
            }
            if (file) {
                file.close();
            }
            return {};
        }

        std::string content;
        content.reserve(static_cast<std::size_t>(file.fileSize()));
        std::uint8_t buffer[512];
        while (file.available()) {
            const int bytesRead = file.read(buffer, sizeof(buffer));
            if (bytesRead <= 0) break;
            content.append(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(bytesRead));
        }
        file.close();
        return content;
    }

    std::string readRange(const std::string& path, std::size_t offset, std::size_t count) override {
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!file || file.isDir() || offset > file.fileSize() || !file.seekSet(offset)) {
            if (file) file.close();
            return {};
        }
        const auto available = static_cast<std::size_t>(file.fileSize()) - offset;
        const std::size_t toRead = count < available ? count : available;
        std::string result(toRead, '\0');
        if (toRead != 0) {
            const int bytesRead = file.read(reinterpret_cast<std::uint8_t*>(&result[0]), toRead);
            result.resize(bytesRead > 0 ? static_cast<std::size_t>(bytesRead) : 0);
        }
        file.close();
        return result;
    }

    bool resourceExists(const std::string& path) const override {
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!file) {
            return false;
        }
        const bool exists = !file.isDir();
        file.close();
        return exists;
    }

    ResourceDirectoryListing listDirectory(
        const std::string& path,
        std::size_t maxEntries) const override {
        ResourceDirectoryListing result;
        File32 directory = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!directory || !directory.isDir()) {
            if (directory) {
                directory.close();
            }
            return result;
        }

        result.ok = true;
        File32 entry;
        while (entry.openNext(&directory, O_RDONLY)) {
            if (result.entries.size() >= maxEntries) {
                result.truncated = true;
                entry.close();
                break;
            }

            char nameBuffer[256]{};
            entry.getName(nameBuffer, sizeof(nameBuffer));

            ResourceEntry resourceEntry;
            resourceEntry.name = nameBuffer;
            resourceEntry.isDirectory = entry.isDir();
            if (!resourceEntry.isDirectory) {
                resourceEntry.size = static_cast<std::size_t>(entry.fileSize());
            }
            result.entries.push_back(std::move(resourceEntry));
            entry.close();
        }
        directory.close();
        return result;
    }
};

#endif // BLASTER_STM32F411_SD_TEXT_RESOURCE_LOADER_H
