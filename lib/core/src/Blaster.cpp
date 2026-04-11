
#include <string>

#include "core/Blaster.h"
#include "core/audio/IAudioEngine.h"

namespace {
    const WeaponProfile& invalidWeapon() {
        static const WeaponProfile weapon{
            .name = "invalid",
            .file = "",
            .category = "",
            .numRounds = 0,
            .boltLockFile = "",
            .emptyFile = "",
            .reloadFile = ""
        };
        return weapon;
    }

    const SoundBank& invalidBank() {
        static const SoundBank bank{
            .name = "invalid",
            .weapons = {}
        };
        return bank;
    }
}

Blaster::Blaster(PlatformServices& services, const std::vector<SoundBank>& soundBanks)
    : m_services(services), m_banks(soundBanks) {
    resetAmmoForCurrentWeapon();
}

bool Blaster::update() {
    auto& input = m_services.input;

    if (input->wasQuitPressed()) {
        return false;
    }

    if (input->wasReloadPressed()) {
        reload();
    } else if (input->wasTriggerPressed()) {
        fire();
    } else if (input->wasNextShortPressed()) {
        nextWeapon();
    } else if (input->wasPrevShortPressed()) {
        prevWeapon();
    } else if (input->wasNextLongPressed()) {
        nextBank();
    } else if (input->wasPrevLongPressed()) {
        prevBank();
    }

    return true;
}

void Blaster::fire() {
    m_services.debug->log("Blaster::fire()");

    const auto& weapon = currentWeapon();
    if (weapon.file.empty()) {
        m_services.debug->error("Blaster: no valid weapon selected");
        return;
    }

    // Empty-magazine behavior for finite-ammo weapons.
    if (!hasInfiniteAmmo() && m_roundsRemaining <= 0) {
        if (!weapon.emptyFile.empty()) {
            m_services.audio->playSound(m_services.assetRoot + weapon.emptyFile);
        } else {
            m_services.debug->log("Blaster: empty trigger pull");
        }
        return;
    }

    // Fire the main shot.
    m_services.audio->playSound(m_services.assetRoot + weapon.file);

    // Infinite ammo: nothing else to track.
    if (hasInfiniteAmmo()) {
        return;
    }

    --m_roundsRemaining;
    m_services.debug->log("Blaster: rounds remaining = " + std::to_string(m_roundsRemaining));

    // Last round fired: trigger bolt lock sound immediately.
    // Current audio engine may interrupt rather than overlap.
    if (m_roundsRemaining == 0 && !weapon.boltLockFile.empty()) {
        m_services.audio->playSound(m_services.assetRoot + weapon.boltLockFile);
    }
}

void Blaster::reload() {
    m_services.debug->log("Blaster::reload()");

    const auto& weapon = currentWeapon();
    if (weapon.file.empty()) {
        m_services.debug->error("Blaster: no valid weapon selected for reload");
        return;
    }

    if (hasInfiniteAmmo()) {
        m_services.debug->log("Blaster: reload ignored for infinite-ammo weapon");
        return;
    }

    m_roundsRemaining = magazineCapacity();
    m_services.debug->log("Blaster: reloaded to " + std::to_string(m_roundsRemaining) + " rounds");

    if (!weapon.reloadFile.empty()) {
        m_services.audio->playSound(m_services.assetRoot + weapon.reloadFile);
    }
}

void Blaster::nextWeapon() {
    m_services.debug->log("Blaster::nextWeapon()");

    const auto& currentWeapons = weapons();
    if (currentWeapons.empty()) {
        m_services.debug->error("Blaster: no weapons in current bank");
        return;
    }

    m_currentWeaponIndex = (m_currentWeaponIndex + 1) % currentWeapons.size();
    resetAmmoForCurrentWeapon();

    m_services.debug->log("Blaster: switched to weapon " + currentWeapons[m_currentWeaponIndex].name);
}

void Blaster::prevWeapon() {
    m_services.debug->log("Blaster::prevWeapon()");

    const auto& currentWeapons = weapons();
    if (currentWeapons.empty()) {
        m_services.debug->error("Blaster: no weapons in current bank");
        return;
    }

    m_currentWeaponIndex =
        (m_currentWeaponIndex == 0) ? (currentWeapons.size() - 1) : (m_currentWeaponIndex - 1);

    resetAmmoForCurrentWeapon();

    m_services.debug->log("Blaster: switched to weapon " + currentWeapons[m_currentWeaponIndex].name);
}

void Blaster::nextBank() {
    m_services.debug->log("Blaster::nextBank()");

    if (m_banks.empty()) {
        m_services.debug->error("Blaster: no sound banks configured");
        return;
    }

    m_currentBankIndex = (m_currentBankIndex + 1) % m_banks.size();
    m_currentWeaponIndex = 0;
    resetAmmoForCurrentWeapon();

    m_services.debug->log("Blaster: switched to bank " + currentBank().name);
}

void Blaster::prevBank() {
    m_services.debug->log("Blaster::prevBank()");

    if (m_banks.empty()) {
        m_services.debug->error("Blaster: no sound banks configured");
        return;
    }

    m_currentBankIndex =
        (m_currentBankIndex == 0) ? (m_banks.size() - 1) : (m_currentBankIndex - 1);
    m_currentWeaponIndex = 0;
    resetAmmoForCurrentWeapon();

    m_services.debug->log("Blaster: switched to bank " + currentBank().name);
}

const SoundBank& Blaster::currentBank() const {
    if (m_banks.empty()) {
        m_services.debug->error("Blaster::currentBank(): no sound banks configured");
        return invalidBank();
    }

    if (m_currentBankIndex >= m_banks.size()) {
        m_services.debug->error("Blaster::currentBank(): bank index out of range");
        return m_banks.front();
    }

    return m_banks[m_currentBankIndex];
}

const std::vector<WeaponProfile>& Blaster::weapons() const {
    return currentBank().weapons;
}

const WeaponProfile& Blaster::currentWeapon() const {
    const auto& currentWeapons = weapons();

    if (currentWeapons.empty()) {
        m_services.debug->error("Blaster::currentWeapon(): no weapons configured");
        return invalidWeapon();
    }

    if (m_currentWeaponIndex >= currentWeapons.size()) {
        m_services.debug->error("Blaster::currentWeapon(): weapon index out of range");
        return currentWeapons.front();
    }

    return currentWeapons[m_currentWeaponIndex];
}

bool Blaster::hasInfiniteAmmo() const {
    return currentWeapon().numRounds <= 0;
}

int Blaster::magazineCapacity() const {
    const int rounds = currentWeapon().numRounds;
    return (rounds > 0) ? rounds : 0;
}

void Blaster::resetAmmoForCurrentWeapon() {
    if (hasInfiniteAmmo()) {
        m_roundsRemaining = 0;
    } else {
        m_roundsRemaining = magazineCapacity();
    }
}