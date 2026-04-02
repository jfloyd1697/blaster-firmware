#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "Platform.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"
#include "core/weapons/WeaponBehaviorController.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class Blaster {
public:
    Blaster(PlatformServices& services,
            std::vector<SoundBank> banks);
    ~Blaster() = default;

    bool update();

protected:
    void handleWeaponSelectionInput();
    void handleReloadInput() const;
    void handleTriggerInput() const;

    void selectNextWeapon();
    void selectPreviousWeapon();
    void reloadCurrentWeapon();

    void equipCurrentWeapon();
    void emitShot() const;
    void flashMuzzle() const;

    [[nodiscard]] bool shouldQuit() const;
    [[nodiscard]] const WeaponProfile* currentWeapon() const;

private:
    PlatformServices& m_services;
    std::vector<SoundBank> m_banks;

    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;

    const WeaponProfile* m_currentProfile = nullptr;
    int m_currentAmmo = 0;

    std::optional<WeaponBehaviorDef> m_behaviorDef;
    std::unique_ptr<WeaponBehaviorController> m_behaviorController;
};

#endif // BLASTER_FIRMWARE_BLASTER_H