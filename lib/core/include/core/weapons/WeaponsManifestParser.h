#ifndef BLASTER_FIRMWARE_WEAPONSMANIFESTPARSER_H
#define BLASTER_FIRMWARE_WEAPONSMANIFESTPARSER_H

#pragma once

#include <string>

#include "core/debug/IDebug.h"
#include "core/weapons/WeaponsManifest.h"

class WeaponsManifestParser {
public:
    static WeaponsManifest parseFromText(const std::string &jsonText, IDebug *debug);
};

#endif // BLASTER_FIRMWARE_WEAPONSMANIFESTPARSER_H
