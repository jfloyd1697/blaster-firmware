//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <vector>

#include "core/Platform.h"
#include "core/weapons/WeaponProfile.h"

class Blaster {
public:
    Blaster(PlatformServices& services, const std::vector<SoundBank>& soundBanks);

    bool update();

    void fire();
    void reload();

    void nextWeapon();
    void prevWeapon();

    void nextBank();
    void prevBank();

    const SoundBank& currentBank() const;
    const WeaponProfile& currentWeapon() const;
    const std::vector<WeaponProfile>& weapons() const;

private:
    bool hasInfiniteAmmo() const;
    int magazineCapacity() const;
    void resetAmmoForCurrentWeapon();

    PlatformServices& m_services;
    std::vector<SoundBank> m_banks;

    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;

    // For finite-ammo weapons only.
    // For infinite-ammo weapons, this value is ignored.
    int m_roundsRemaining = 0;
};
#endif //BLASTER_FIRMWARE_BLASTER_H
