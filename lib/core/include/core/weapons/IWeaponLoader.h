//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_WEAPONJSONLOADER_H
#define BLASTER_WEAPONJSONLOADER_H

#pragma once

#include <string>
#include <vector>

#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"
#include "core/text_resource_loader/ITextResourceLoader.h"

class IWeaponLoader {
public:
    explicit IWeaponLoader(ITextResourceLoader *textLoader) : m_text(textLoader) {};
    virtual ~IWeaponLoader() = default;
    std::vector<SoundBank> loadSoundBanks(const std::string &jsonFilePath) const;

protected:
    std::string loadText(const std::string& path) const {;
        if (m_text == nullptr) {
            return {};
        }
        return m_text->loadText(path);
    }

    static std::vector<SoundBank> parseSoundBanksJson(const std::string &jsonText);
    ITextResourceLoader *m_text = nullptr;
};

#endif //BLASTER_WEAPONJSONLOADER_H
