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

#include "core/debug/IDebug.h"

class EspSdTextResourceLoader : public ITextResourceLoader {
public:
    explicit EspSdTextResourceLoader(IDebug *debug): ITextResourceLoader(debug) {};

    std::string loadText(const std::string& path) override {
        File file = SD.open(path.c_str(), FILE_READ);
        if (!file) {
            debug->error("EspSdTextResourceLoader: failed to open: " + path);
        }

        String content;
        while (file.available()) {
            content += static_cast<char>(file.read());
        }
        file.close();

        return std::string(content.c_str());
    }
};


#endif //BLASTER_FIRMWARE_ESPTEXTRESOURCELOADER_H