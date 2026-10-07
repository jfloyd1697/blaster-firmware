#ifndef BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H
#define BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H

#pragma once

#include <SD.h>

#include <string>

#include "core/debug/IDebug.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

class TeensySdTextResourceLoader : public ITextResourceLoader {
public:
    explicit TeensySdTextResourceLoader(IDebug* debug)
        : ITextResourceLoader(debug) {
    }

    std::string loadText(const std::string& path) override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file) {
            if (m_debug != nullptr) {
                m_debug->error("TeensySdTextResourceLoader: failed to open: " + path);
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
};

#endif // BLASTER_TEENSY_SD_TEXT_RESOURCE_LOADER_H
