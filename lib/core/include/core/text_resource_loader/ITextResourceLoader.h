#ifndef BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H
#define BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H

#pragma once

#include <string>

#include "core/debug/IDebug.h"

class ITextResourceLoader {
public:
    explicit ITextResourceLoader(IDebug *debug) : m_debug(debug) {
    };

    virtual ~ITextResourceLoader() = default;

    virtual std::string loadText(const std::string &path) = 0;

    ITextResourceLoader() = default;

    IDebug *m_debug;
};

#endif // BLASTER_FIRMWARE_ITEXTRESOURCELOADER_H
