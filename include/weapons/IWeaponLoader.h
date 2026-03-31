//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_WEAPONJSONLOADER_H
#define BLASTER_WEAPONJSONLOADER_H

#pragma once

#include <string>
#include <vector>
#include "weapons/WeaponProfile.h"

class IWeaponLoader {
public:
    virtual ~IWeaponLoader() = default;
    std::vector<SoundBank> loadSoundBanks(const std::string &jsonFilePath);

protected:
    virtual std::string loadText(const std::string& path) = 0;

    static std::vector<SoundBank> parseSoundBanksJson(const std::string &jsonText);

};

#endif //BLASTER_WEAPONJSONLOADER_H
