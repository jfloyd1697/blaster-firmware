//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

// Blaster.h
#pragma once
#include <vector>
#include "weapons/WeaponProfile.h"
#include "../include/Platform.h"

class Blaster {
public:
    explicit Blaster(PlatformServices &services, const std::vector<SoundBank> &weapons = {});

    // Fire the current weapon
    void fire() const;

    // Switch weapons
    void nextWeapon();

    void prevWeapon();

    void nextBank();

    void prevBank();

    bool update();

    // Get the current weapon info
    [[nodiscard]] const WeaponProfile &currentWeapon() const;

    [[nodiscard]] const SoundBank &currentBank() const;

private:
    const std::vector<WeaponProfile> &weapons() const;
    PlatformServices &m_services;
    std::vector<SoundBank> m_banks;
    size_t m_currentWeaponIndex = 0;
    size_t m_currentBankIndex = 0;
};

#endif //BLASTER_FIRMWARE_BLASTER_H
