#ifndef BLASTER_FIRMWARE_RESOURCETREEDUMPER_H
#define BLASTER_FIRMWARE_RESOURCETREEDUMPER_H

#pragma once

#include <cstddef>
#include <string>

class ITextResourceLoader;
struct IDebug;

struct ResourceTreeDumpOptions {
    int maxDepth = 3;
    std::size_t maxEntriesPerDirectory = 16;
};

class ResourceTreeDumper {
public:
    static void dump(
        const ITextResourceLoader& loader,
        IDebug& debug,
        const std::string& rootPath = "/",
        const ResourceTreeDumpOptions& options = {});
};

#endif // BLASTER_FIRMWARE_RESOURCETREEDUMPER_H
