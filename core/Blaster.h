//
// Created by user on 3/26/2026.
//

#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

// Blaster.h
#pragma once
#include <vector>
#include "WeaponProfile.h"
#include "Platform.h"

class Blaster {
public:
    explicit Blaster(PlatformServices &services, const std::vector<WeaponProfile> &weapons = {});

    // Fire the current weapon
    void fire() const;

    // Switch weapons
    void nextWeapon();
    void prevWeapon();
    bool update();

    // Get the current weapon info
    const WeaponProfile& currentWeapon() const;
    std::vector<WeaponProfile> weapons();

private:
    PlatformServices &m_services;
    std::vector<WeaponProfile> m_weapons;
    size_t m_currentIndex = 0;
};

#endif //BLASTER_FIRMWARE_BLASTER_H