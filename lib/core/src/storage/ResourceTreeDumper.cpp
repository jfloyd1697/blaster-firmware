#include "core/storage/ResourceTreeDumper.h"

#include <string>

#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

namespace {
std::string joinPath(const std::string& parent, const std::string& child) {
    if (parent.empty() || parent == "/") {
        return "/" + child;
    }

    if (parent.back() == '/') {
        return parent + child;
    }
    return parent + "/" + child;
}

void dumpDirectory(
    const ITextResourceLoader& loader,
    IDebug& debug,
    const std::string& path,
    int depth,
    const ResourceTreeDumpOptions& options) {
    if (depth > options.maxDepth) {
        return;
    }

    const ResourceDirectoryListing listing = loader.listDirectory(path, options.maxEntriesPerDirectory);
    if (!listing.ok) {
        debug.error("Resource tree: failed to open directory " + path);
        return;
    }

    const std::string indent(static_cast<std::size_t>(depth * 2), ' ');
    for (const ResourceEntry& entry : listing.entries) {
        debug.log(indent + (entry.isDirectory ? "[DIR]  " : "[FILE] ") + entry.name);

        if (entry.isDirectory && depth < options.maxDepth) {
            dumpDirectory(loader, debug, joinPath(path, entry.name), depth + 1, options);
        }
    }

    if (listing.truncated) {
        debug.log(indent + "...");
    }
}
}

void ResourceTreeDumper::dump(
    const ITextResourceLoader& loader,
    IDebug& debug,
    const std::string& rootPath,
    const ResourceTreeDumpOptions& options) {
    debug.log("Storage structure:");
    debug.log("[DIR]  " + rootPath);
    dumpDirectory(loader, debug, rootPath, 1, options);
    debug.log("End storage structure");
}
