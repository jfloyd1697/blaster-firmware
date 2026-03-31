//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_WEAPONJSONLOADER_H
#define BLASTER_WEAPONJSONLOADER_H


#pragma once
#include <string>
#include <vector>

#include "WeaponProfile.h"

std::vector<SoundBank> loadSoundBanks(const std::string& jsonFilePath);

#endif //BLASTER_WEAPONJSONLOADER_H