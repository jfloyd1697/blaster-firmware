//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H

#pragma once

#include "weapon_behavior/text_resource_loader/ITextResourceLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/debug/IDebug.h"

class PCTextResourceLoader : public ITextResourceLoader {
public:
    explicit PCTextResourceLoader(IDebug *debug): ITextResourceLoader(debug) {};
    std::string loadText(const std::string& path) override {
        if (!std::filesystem::exists(path)) {
            debug->error("PCTextResourceLoader: file not found: " + path);
        }

        const std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in) {
            debug->error("PCTextResourceLoader: failed to open: " + path);
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }
};

#endif //BLASTER_FIRMWARE_PCTEXTRESOURCELOADER_H