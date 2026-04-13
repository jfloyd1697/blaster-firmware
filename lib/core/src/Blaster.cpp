#include "core/Blaster.h"

#include <memory>
#include <cstdlib>
#include <utility>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"

#include "weapon_behavior/ShootContext.h"
#include "weapon_behavior/WeaponBehaviorEventHandler.h"
#include "weapon_behavior/WeaponBehaviorLoadHelpers.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"


using namespace weapon_behavior;

Blaster::Blaster(PlatformServices &services,
                 std::vector<SoundBank> banks)
    : m_services(services),
      m_banks(std::move(banks)) {
    equipCurrentWeapon();
}

bool Blaster::update() {
    if (m_banks.empty()) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::update: no sound banks loaded");
        }
        return false;
    }

    handleWeaponSelectionInput();
    handleReloadInput();
    handleTriggerInput();

    if (m_controller) {
        m_controller->update();
    }

    return !shouldQuit();
}

void Blaster::handleWeaponSelectionInput() {
    if (!m_services.input) {
        return;
    }

    if (m_services.input->wasNextShortPressed()) {
        selectNextWeapon();
    }

    if (m_services.input->wasPrevShortPressed()) {
        selectPreviousWeapon();
    }
}

void Blaster::handleReloadInput() {
    if (!m_services.input || !m_controller) {
        return;
    }

    if (m_services.input->wasReloadPressed()) {
        m_controller->reloadWeapon();
    }

    if (!m_currentProfile) {
        return;
    }

    m_currentAmmo = m_currentProfile->magazineSize;

    if (m_services.debug) {
        m_services.debug->log("Reloaded weapon: " + m_currentProfile->name);
    }
}

void Blaster::handleTriggerInput() const {
    if (!m_services.input || !m_controller) {
        return;
    }

    if (m_services.input->wasTriggerPressed()) {
        m_controller->onTriggerPressed();
    }

    if (m_services.input->isTriggerHeld()) {
        m_controller->onTriggerHeld();
    }

    if (m_services.input->wasTriggerReleased()) {
        m_controller->onTriggerReleased();
    }

}

void Blaster::selectNextWeapon() {
    if (m_banks.empty()) {
        return;
    }

    const SoundBank &bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return;
    }

    m_currentWeaponIndex = (m_currentWeaponIndex + 1) % bank.weapons.size();
    equipCurrentWeapon();
}

void Blaster::selectPreviousWeapon() {
    if (m_banks.empty()) {
        return;
    }

    const SoundBank &bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return;
    }

    m_currentWeaponIndex =
            (m_currentWeaponIndex == 0)
                ? (bank.weapons.size() - 1)
                : (m_currentWeaponIndex - 1);

    equipCurrentWeapon();
}

void Blaster::equipCurrentWeapon() {
    m_currentProfile = currentWeapon();

    if (!m_currentProfile) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no weapon available");
        }
        m_controller.reset();
        return;
    }
    m_controller->equipWeapon(*m_currentProfile);

    if (m_services.debug) {
        m_services.debug->log("Equipped weapon: " + m_currentProfile->name);
    }
}

const WeaponProfile *Blaster::currentWeapon() const {
    if (m_banks.empty()) {
        return nullptr;
    }

    if (m_currentBankIndex >= m_banks.size()) {
        return nullptr;
    }

    const SoundBank &bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return nullptr;
    }

    if (m_currentWeaponIndex >= bank.weapons.size()) {
        return nullptr;
    }

    return &bank.weapons[m_currentWeaponIndex];
}

bool Blaster::shouldQuit() const {
    if (!m_services.input) {
        return false;
    }

    return m_services.input->wasQuitPressed();
}
