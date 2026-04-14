#include "core/Blaster.h"

#include <memory>
#include <cstdlib>
#include <utility>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/weapons/ShootContext.h"
#include "core/weapons/SoundBank.h"
#include "core/weapons/WeaponProfile.h"
#include "core/weapons//WeaponBehaviorController.h"
#include "core/weapons/WeaponBehaviorLoadHelpers.h"
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

    if (m_behaviorController) {
        m_behaviorController->update();
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

void Blaster::handleReloadInput() const {
    if (!m_services.input || !m_behaviorController) {
        return;
    }

    if (m_services.input->wasReloadPressed()) {
        m_behaviorController->handleEvent("reload");
    }
}

void Blaster::handleTriggerInput() const {
    if (!m_services.input || !m_behaviorController) {
        return;
    }

    if (m_services.input->wasTriggerPressed()) {
        m_behaviorController->handleEvent("trigger_pressed");
    }

    if (m_services.input->wasTriggerReleased()) {
        m_behaviorController->handleEvent("trigger_released");
    }

    if (m_services.input->isTriggerHeld()) {
        m_behaviorController->handleEvent("trigger_held");
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

void Blaster::reloadCurrentWeapon() {
    if (!m_currentProfile) {
        return;
    }

    m_currentAmmo = m_behaviorDef->magazineSize;

    if (m_services.debug) {
        m_services.debug->log("Reloaded weapon: " + m_currentProfile->name);
    }

    if (m_behaviorController) {
        m_behaviorController->handleEvent("reload_complete");
    }
}

void Blaster::equipCurrentWeapon() {
    m_currentProfile = currentWeapon();

    if (!m_currentProfile) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no weapon available");
        }
        m_behaviorDef.reset();
        m_behaviorController.reset();
        return;
    }

    m_currentAmmo = m_behaviorDef->magazineSize;

    if (!m_services.text_loader) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no text loader available");
        }
        m_behaviorDef.reset();
        m_behaviorController.reset();
        return;
    }

    try {
        m_behaviorDef = loadWeaponBehavior(*m_services.text_loader, m_currentProfile->behaviorPath);
    } catch (const std::exception &ex) {
        if (m_services.debug) {
            m_services.debug->error(
                "Blaster::equipCurrentWeapon: failed to load behavior for " +
                m_currentProfile->name + ": " + ex.what());
        }
        m_behaviorDef.reset();
        m_behaviorController.reset();
        return;
    }

    ShootContext ctx;
    ctx.time = m_services.time.get();
    ctx.audio = m_services.audio.get();
    ctx.debug = m_services.debug.get();
    ctx.profile = m_currentProfile;
    ctx.ammo = &m_currentAmmo;

    ctx.emitShot = [this] {
        emitShot();
    };

    ctx.flashMuzzle = [this] {
        flashMuzzle();
    };

    ctx.playSound = [this](const std::string &path, bool loop) {
        if (!m_services.audio || path.empty()) {
            return;
        }
        m_services.audio->playSound(path, loop);
    };

    ctx.playRandomSound = [this](const std::vector<std::string> &sounds, bool loop) {
        if (!m_services.audio || sounds.empty()) {
            return;
        }

        const std::size_t index = static_cast<std::size_t>(std::rand()) % sounds.size();
        m_services.audio->playSound(sounds[index], loop);
    };

    ctx.stopSound = [this]() {
        if (!m_services.audio) {
            return;
        }
        m_services.audio->stop();
    };

    ctx.setLight = [this](const LightPatternDef &pattern) {
        if (!m_services.lights) {
            return;
        }
        m_services.lights->setPattern(std::make_shared<LightPatternDef>(pattern));
    };

    ctx.flashLight = [this](const LightPatternDef &) {
        if (!m_services.lights) {
            return;
        }
        m_services.lights->flash();
    };

    ctx.emitBehaviorEvent = [this](const std::string &event) {
        if (m_behaviorController) {
            m_behaviorController->handleEvent(event);
        }
    };

    m_behaviorController = std::make_unique<WeaponBehaviorController>(
        *m_behaviorDef,
        std::move(ctx));

    m_behaviorController->initialize();

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

void Blaster::emitShot() const {
    if (!m_currentProfile) {
        return;
    }

    if (m_services.debug) {
        m_services.debug->log(
            "Shot fired: " + m_currentProfile->name +
            ", ammo remaining: " + std::to_string(m_currentAmmo));
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
