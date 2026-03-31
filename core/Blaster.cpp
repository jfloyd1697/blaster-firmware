#include "Blaster.h"

#include <stdexcept>
#include <string>

#include "IAudioEngine.h"

Blaster::Blaster(PlatformServices &services, const std::vector<SoundBank> &soundBanks)
    : m_services(services), m_banks(soundBanks) {
}

bool Blaster::update() {
    auto &input = m_services.input;
    auto &audio = m_services.audio;

    input ->update();

    if (input->wasQuitPressed()) {
        return false;
    }

    if (input->wasTriggerPressed()) {
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

    audio -> update();

    return true;
}

void Blaster::fire() const {
    m_services.debug->log("Blaster::fire()");
    const auto &[name, file, category] = currentWeapon();
    const std::string fullPath = m_services.assetRoot + file;

    m_services.debug->log("Blaster: Current weapon index: " + std::to_string(m_currentWeaponIndex));
    m_services.debug->log("Blaster: Firing weapon: " + name);

    m_services.audio->playSound(fullPath);
}

void Blaster::nextWeapon() {
    m_services.debug->log("Blaster::nextWeapon()");

    if (weapons().empty()) {
        return;
    }

    m_currentWeaponIndex = (m_currentWeaponIndex + 1) % weapons().size();
    m_services.debug->log("Blaster: Switched to next weapon: " + weapons()[m_currentWeaponIndex].name);
}

void Blaster::prevWeapon() {
    m_services.debug->log("Blaster::prevWeapon()");

    if (weapons().empty()) {
        return;
    }

    if (m_currentWeaponIndex == 0) {
        m_currentWeaponIndex = weapons().size() - 1;
    } else {
        --m_currentWeaponIndex;
    }

    m_services.debug->log("Blaster: Switched to previous weapon: " + weapons()[m_currentWeaponIndex].name);
}

const WeaponProfile &Blaster::currentWeapon() const {
    if (weapons().empty()) {
        throw std::runtime_error("Blaster::currentWeapon() called with no weapons configured.");
    }

    if (m_currentWeaponIndex >= weapons().size()) {
        throw std::runtime_error("Blaster::currentWeapon() index out of range.");
    }

    return weapons()[m_currentWeaponIndex];
}

void Blaster::nextBank() {
    m_services.debug->log("Blaster::nextBank()");

    if (m_banks.empty()) {
        return;
    }

    m_currentBankIndex = (m_currentBankIndex + 1) % m_banks.size();
    m_currentWeaponIndex = 0; // Reset weapon index when switching banks
    m_services.debug->log("Blaster: Switched to next bank: " + m_banks[m_currentBankIndex].name);

}

void Blaster::prevBank() {
    m_services.debug->log("Blaster::nextBank()");

    if (m_banks.empty()) {
        return;
    }

    if (m_currentBankIndex == 0) {
        m_currentBankIndex = m_banks.size() - 1;
    } else {
        --m_currentBankIndex;
    }

    m_currentWeaponIndex = 0; // Reset weapon index when switching banks

    m_services.debug->log("Blaster: Switched to next bank: " + currentBank().name);
}

const SoundBank &Blaster::currentBank() const {
    if (m_banks.empty()) {
        throw std::runtime_error("Blaster::currentBank() called with no sound banks configured.");
    }

    if (m_currentBankIndex >= m_banks.size()) {
        throw std::runtime_error("Blaster::currentBank() index out of range.");
    }

    return m_banks[m_currentBankIndex];
}

const std::vector<WeaponProfile> & Blaster::weapons() const {
    if (m_banks.empty()) {
        static const std::vector<WeaponProfile> emptyWeapons;
        return emptyWeapons;
    }
    return m_banks[m_currentBankIndex].weapons;
}
