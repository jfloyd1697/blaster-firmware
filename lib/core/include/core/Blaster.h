#ifndef BLASTER_FIRMWARE_BLASTER_H
#define BLASTER_FIRMWARE_BLASTER_H

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Platform.h"
#include "weapons/SoundBank.h"
#include "weapons/IShootMode.h"
#include "weapons/WeaponProfile.h"


class Blaster {
public:
    Blaster(PlatformServices& services,
            std::vector<SoundBank> banks);
    ~Blaster() = default;

    bool update();

protected:
    void handleWeaponSelectionInput();
    void handleReloadInput();
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
    std::unique_ptr<IShootMode> m_shootMode;

    PlatformServices& m_services;
    std::vector<SoundBank> m_banks;

    std::size_t m_currentBankIndex = 0;
    std::size_t m_currentWeaponIndex = 0;

    const WeaponProfile* m_currentProfile = nullptr;

    int m_currentAmmo = 0;
};
#endif //BLASTER_FIRMWARE_BLASTER_H
