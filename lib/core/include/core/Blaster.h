#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "core/Platform.h"
#include "core/weapons/SoundBank.h"

class WeaponBehaviorController;

class Blaster {
public:
    Blaster(PlatformServices& services, std::vector<WeaponBank> banks);
    ~Blaster();

    bool update();

    // Executes deferred weapon loading outside the control tick on STM32.
    void servicePendingWeaponLoad();
    [[nodiscard]] bool hasPendingWeaponLoad() const { return m_weaponLoadPending; }

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

    [[nodiscard]] bool shouldQuit() const;
    [[nodiscard]] const WeaponEntry* currentWeapon() const;

private:
    PlatformServices& m_services;
    std::vector<WeaponBank> m_banks;
    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;
    std::optional<weapon_behavior::WeaponBehaviorDef> m_currentBehavior;
    std::unique_ptr<WeaponBehaviorController> m_behaviorController;
    bool m_weaponLoadPending = false;
};

#endif // BLASTER_FIRMWARE_BLASTER_H
