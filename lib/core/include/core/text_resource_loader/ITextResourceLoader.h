#ifndef BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/debug/IDebug.h"

struct ResourceEntry {
    std::string name;
    bool isDirectory = false;
    std::size_t size = 0;
};

struct ResourceDirectoryListing {
    bool ok = false;
    bool truncated = false;
    std::vector<ResourceEntry> entries;
};

class ITextResourceLoader {
public:
    explicit ITextResourceLoader(IDebug* debug) : m_debug(debug) {
    }

    virtual ~ITextResourceLoader() = default;

    virtual std::string loadText(const std::string& path) = 0;
    // Read a bounded range from a binary SD file. Empty result indicates EOF/error.
    virtual std::string readRange(const std::string& path, std::size_t offset, std::size_t count) {
        // A platform must override this for efficient streaming; never load the whole file here.
        (void)path;
        (void)offset;
        (void)count;
        return {};
    }

    virtual bool resourceExists(const std::string& path) const = 0;
    virtual ResourceDirectoryListing listDirectory(
        const std::string& path,
        std::size_t maxEntries) const {
        (void)path;
        (void)maxEntries;
        return {};
    }

    ITextResourceLoader() = default;

protected:
    IDebug* m_debug = nullptr;
};

#endif // BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H
