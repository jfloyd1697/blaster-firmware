#ifndef BLASTER_FIRMWARE_WEAPONLOADER_H
#define BLASTER_FIRMWARE_WEAPONLOADER_H

#pragma once

#include <string>
#include <vector>

#include "core/debug/IDebug.h"

struct WeaponBank;
class ITextResourceLoader;

class WeaponLoader {
public:
    static std::vector<WeaponBank> loadBanks(
        ITextResourceLoader& loader,
        IDebug& debug,
        const std::string& manifestPath);
};

#endif // BLASTER_FIRMWARE_WEAPONLOADER_H
