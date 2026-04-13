//
// Created by user on 4/13/2026.
//

#ifndef BLASTER_FIRMWARE_PCBASICWEAPONLOADER_H
#define BLASTER_FIRMWARE_PCBASICWEAPONLOADER_H

#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>

#include "BasicWeaponLoader.h"


class PCBasicWeaponLoader : public BasicWeaponLoader {
public:
    explicit PCBasicWeaponLoader(IDebug *debug) : BasicWeaponLoader(), m_debug(debug) {};
    ~PCBasicWeaponLoader() override = default;

protected:
    std::string loadText(const std::string& path) override {
        std::ifstream file(path);
        if (!file.is_open()) {
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
private:
    IDebug *m_debug = nullptr;
};


#endif //BLASTER_FIRMWARE_PCBASICWEAPONLOADER_H