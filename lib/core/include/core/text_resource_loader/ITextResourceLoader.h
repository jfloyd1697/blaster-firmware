//
// Created by user on 4/2/2026.
//

#ifndef BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H

#pragma once

#include <string>

struct IDebug;

class ITextResourceLoader {
public:
    explicit ITextResourceLoader(IDebug *debug) : debug(debug) {};
    virtual ~ITextResourceLoader() = default;
    virtual std::string loadText(const std::string& path) = 0;
protected:
    IDebug *debug;
};

#endif //BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H