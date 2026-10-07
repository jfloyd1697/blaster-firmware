#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/Platform.h"
#include "core/weapons/SoundBank.h"

class WeaponBehaviorController;

class Blaster {
public:
    Blaster(PlatformServices& services,
            std::vector<WeaponBank> banks);
    ~Blaster();

    bool update();

protected:
    void handleWeaponSelectionInput();
    void handleReloadInput();
    void handleTriggerInput();

    void selectNextWeapon();
    void selectPreviousWeapon();
    void reloadCurrentWeapon();

    void selectNextBank();
    void selectPreviousBank();

    void equipCurrentWeapon();
    void emitShot() const;
    void flashMuzzle() const;


    [[nodiscard]] bool shouldQuit() const;
    [[nodiscard]] const WeaponEntry* currentWeapon() const;

private:
    PlatformServices& m_services;
    std::vector<WeaponBank> m_banks;

    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;
    std::optional<weapon_behavior::WeaponBehaviorDef> m_currentBehavior;
    int m_currentAmmo = 0;

    std::unique_ptr<WeaponBehaviorController> m_behaviorController;
};

#endif // BLASTER_FIRMWARE_BLASTER_H
