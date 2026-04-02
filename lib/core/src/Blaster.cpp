#include "core/Blaster.h"

#include <utility>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/weapons/IShootMode.h"
#include "core/weapons/ShootContext.h"
#include "core/weapons/ShootModeFactory.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"

Blaster::Blaster(PlatformServices& services,
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

    if (m_shootMode) {
        m_shootMode->update();
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
    if (!m_services.input || !m_currentProfile) {
        return;
    }

    if (!m_services.input->wasReloadPressed()) {
        return;
    }

    reloadCurrentWeapon();
}

void Blaster::handleTriggerInput() const {
    if (!m_services.input || !m_shootMode) {
        return;
    }

    if (m_services.input->wasTriggerPressed()) {
        m_shootMode->onTriggerPressed();
    }

    if (m_services.input->wasTriggerReleased()) {
        m_shootMode->onTriggerReleased();
    }
}

void Blaster::selectNextWeapon() {
    if (m_banks.empty()) {
        return;
    }

    const SoundBank& bank = m_banks[m_currentBankIndex];
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

    const SoundBank& bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return;
    }

    m_currentWeaponIndex =
        (m_currentWeaponIndex == 0)
            ? (bank.weapons.size() - 1)
            : (m_currentWeaponIndex - 1);

    equipCurrentWeapon();
}

void Blaster::reloadCurrentWeapon() {
    if (!m_currentProfile) {
        return;
    }

    m_currentAmmo = m_currentProfile->magazineSize;

    if (m_services.debug) {
        m_services.debug->log("Reloaded weapon: " + m_currentProfile->name);
    }

    if (m_services.audio && !m_currentProfile->sounds.reload.empty()) {
        m_services.audio->playSound(m_currentProfile->sounds.reload);
    }
}

void Blaster::equipCurrentWeapon() {
    m_currentProfile = currentWeapon();

    if (!m_currentProfile) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no weapon available");
        }
        m_shootMode.reset();
        return;
    }

    m_currentAmmo = m_currentProfile->magazineSize;

    ShootContext ctx;
    ctx.time = m_services.time.get();
    ctx.audio = m_services.audio.get();
    ctx.debug = m_services.debug.get();
    ctx.profile = m_currentProfile;
    ctx.ammo = &m_currentAmmo;
    ctx.emitShot = [this]() { this->emitShot(); };
    ctx.flashMuzzle = [this]() { this->flashMuzzle(); };

    m_shootMode = createShootMode(ctx);

    if (m_shootMode) {
        m_shootMode->onEquipped();
    }

    if (m_services.debug) {
        m_services.debug->log("Equipped weapon: " + m_currentProfile->name);
    }
}

const WeaponProfile* Blaster::currentWeapon() const {
    if (m_banks.empty()) {
        return nullptr;
    }

    const SoundBank& bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return nullptr;
    }

    if (m_currentWeaponIndex >= bank.weapons.size()) {
        return nullptr;
    }

    return &bank.weapons[m_currentWeaponIndex];
}

void Blaster::emitShot() const {
    if (!m_currentProfile) {
        return;
    }

    if (m_services.debug) {
        m_services.debug->log(
            "Shot fired: " + m_currentProfile->name +
            ", ammo remaining: " + std::to_string(m_currentAmmo)
        );
    }
}

void Blaster::flashMuzzle() const {
    if (!m_services.lights) {
        return;
    }

    m_services.lights->flash();
}

bool Blaster::shouldQuit() const {
    if (!m_services.input) {
        return false;
    }

    return m_services.input->wasQuitPressed();
}