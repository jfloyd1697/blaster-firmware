//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_WEAPONJSONLOADER_H
#define BLASTER_WEAPONJSONLOADER_H

#pragma once

#include <string>
#include <vector>

#include "core/debug/IDebug.h"
#include "core/weapons/SoundBank.h"

class IWeaponLoader {
public:
    virtual ~IWeaponLoader() = default;

    std::vector<SoundBank> loadSoundBanks(const std::string &jsonFilePath, IDebug *debug);

protected:
    virtual std::string loadText(const std::string &path, IDebug *debug) = 0;
};

#endif //BLASTER_WEAPONJSONLOADER_H
