#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Platform.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"
#include "core/weapons/IWeaponController.h"

class Blaster {
public:
    Blaster(PlatformServices& services,
            std::vector<SoundBank> banks);
    ~Blaster() = default;

    bool update();

    void handleWeaponSelectionInput();
    void handleReloadInput();
    void handleTriggerInput() const;

    void selectNextWeapon();
    void selectPreviousWeapon();

    void equipCurrentWeapon();

    [[nodiscard]] bool shouldQuit() const;
    [[nodiscard]] const WeaponProfile* currentWeapon() const;

private:
    PlatformServices& m_services;
    std::vector<SoundBank> m_banks;

    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;

    const WeaponProfile *m_currentProfile = nullptr;
    int m_currentAmmo = 0;

    std::unique_ptr<IWeaponController> m_controller;
};

#endif // BLASTER_FIRMWARE_BLASTER_H