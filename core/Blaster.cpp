// Blaster.cpp
#include "Blaster.h"

#include <iostream>

#include "IAudioEngine.h"


Blaster::Blaster(PlatformServices &services, const std::vector<WeaponProfile> &weapons)
    : m_services(services), m_weapons(weapons)
{
}

bool Blaster::update() {
    auto &input = m_services.input;

    if (input->isTriggerPressed()) {
        fire();
    }
    else if (input->isNextPressed()) {
        nextWeapon();
    }
    else if (input->isPrevPressed()) {
        prevWeapon();
    }
    else if (input->isQuitPressed()) {
        return false;
    }
    return true;
}

void Blaster::fire() const {
    m_services.debug->log("Blaster::fire()");
    m_services.debug->log("Blaster: Current weapon index: " + std::to_string(m_currentIndex));
    if (m_weapons.empty()) {
        m_services.debug->error("Blaster: No weapons available to fire!");
        return;
    }
    const auto&[name, file] = m_weapons[m_currentIndex];
    const std::string fullPath = m_services.assetRoot + file;
    m_services.debug->log("Blaster: Firing weapon: " + name);
    m_services.audio->playSound(fullPath);
}

void Blaster::nextWeapon() {
    m_services.debug->log("Blaster::nextWeapon()");
    if (m_weapons.empty()) return;
    m_currentIndex = (m_currentIndex + 1) % m_weapons.size();
    m_services.debug->log("Blaster: Switched to next weapon: " + m_weapons[m_currentIndex].name);
}

void Blaster::prevWeapon() {
    m_services.debug->log("Blaster::prevWeapon()");
    if (m_weapons.empty()) return;
    if (m_currentIndex == 0)
        m_currentIndex = m_weapons.size() - 1;
    else
        --m_currentIndex;
    m_services.debug->log("Blaster: Switched to previous weapon: " + m_weapons[m_currentIndex].name);
}


const WeaponProfile& Blaster::currentWeapon() const {
    return m_weapons[m_currentIndex];
}

std::vector<WeaponProfile> Blaster::weapons() {
    return m_weapons;
}
