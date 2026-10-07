     #include "core/Blaster.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "core/weapons/ShootContext.h"
#include "core/weapons/WeaponBehaviorController.h"
#include "weapon_behavior/WeaponBehaviorParser.h"

     Blaster::Blaster(PlatformServices &services,
                      std::vector<WeaponBank> banks)
    : m_services(services),
      m_banks(std::move(banks)) {
    equipCurrentWeapon();
}

Blaster::~Blaster() = default;

bool Blaster::update() {
    if (m_banks.empty()) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::update: no banks loaded");
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

    if (m_services.input->wasNextLongPressed()) {
        selectNextBank();
        return;
    }

    if (m_services.input->wasPrevLongPressed()) {
        selectPreviousBank();
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
    if (!m_services.input || !m_behaviorController) {
        return;
    }

    if (m_services.input->wasReloadPressed()) {
        reloadCurrentWeapon();
    }
}

void Blaster::handleTriggerInput() {
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

    const WeaponBank &bank = m_banks[m_currentBankIndex];
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

    const WeaponBank &bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return;
    }

    m_currentWeaponIndex = (m_currentWeaponIndex == 0)
                               ? (bank.weapons.size() - 1)
                               : (m_currentWeaponIndex - 1);

    equipCurrentWeapon();
}

void Blaster::reloadCurrentWeapon() {
    if (!m_currentBehavior) {
        return;
    }
    if (m_behaviorController) {
        m_behaviorController->handleEvent("reload");
    }
    if (m_currentBehavior->magazineSize == 0) {
        m_currentAmmo = -1;
    } else {
        m_currentAmmo = m_currentBehavior->magazineSize;
    }

    if (m_behaviorController) {
        m_behaviorController->handleEvent("reload_complete");
    }

    if (m_services.debug) {
        m_services.debug->log("Reloaded weapon: " + m_currentBehavior->weapon);
    }
}

void Blaster::selectNextBank() {
    if (m_banks.empty()) {
        return;
    }

    m_currentBankIndex = (m_currentBankIndex + 1) % m_banks.size();
    m_currentWeaponIndex = 0;
    equipCurrentWeapon();

    if (m_services.debug) {
        m_services.debug->log("Switched to next bank");
    }
}

void Blaster::selectPreviousBank() {
    if (m_banks.empty()) {
        return;
    }

    m_currentBankIndex =
            (m_currentBankIndex == 0)
                ? (m_banks.size() - 1)
                : (m_currentBankIndex - 1);

    m_currentWeaponIndex = 0;
    equipCurrentWeapon();

    if (m_services.debug) {
        m_services.debug->log("Switched to previous bank");
    }
}


std::string normalizeEspPath(const std::string& path) {
    std::string out = path;

    for (char& ch : out) {
                                                                                                                                                                                                                                   if (ch == '\\') {
            ch = '/';
        }
    }

    // const std::string assetsPrefix = "assets/";
    // if (out.rfind(assetsPrefix, 0) == 0) {
    //     out = "/" + out.substr(assetsPrefix.size());
    // } else if (!out.empty() && out[0] != '/') {
    //     out = "/" + out;
    // }

    return out;
}


void Blaster::equipCurrentWeapon() {
    const WeaponEntry* entry = currentWeapon();
    if (!entry) {
        m_currentBehavior.reset();
        m_behaviorController.reset();
        return;
    }

    if (!m_services.textLoader) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no text loader");
        }
        return;
    }


    const std::string text = m_services.textLoader->loadText(normalizeEspPath(entry->behaviorPath));
    if (text.empty()) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: failed to load " + normalizeEspPath(entry->behaviorPath));
        }
        return;
    }

    const weapon_behavior::WeaponBehaviorDef def =
        weapon_behavior::WeaponBehaviorParser::parseFromText(text, m_services.debug.get());

    m_currentBehavior = def;

    m_currentAmmo = (m_currentBehavior->magazineSize == 0)
        ? -1
        : m_currentBehavior->magazineSize;

    ShootContext ctx;
    ctx.time = m_services.time.get();
    ctx.audio = m_services.audio.get();
    ctx.debug = m_services.debug.get();
    ctx.ammo = &m_currentAmmo;

    ctx.emitShot = [this]() { emitShot(); };
    ctx.flashMuzzle = [this]() { flashMuzzle(); };

    ctx.playSound = [this](const std::string& path, bool loop, bool blocking) {
        if (m_services.audio) {
            m_services.audio->playSound(path, loop, blocking);
        }
    };

    ctx.playRandomSound = [this](const std::vector<std::string>& sounds, bool loop, bool blocking) {
        if (!m_services.audio || sounds.empty()) {
            return;
        }
        const std::size_t index = static_cast<std::size_t>(std::rand()) % sounds.size();
        m_services.audio->playSound(sounds[index], loop, blocking);
    };

    ctx.stopSound = [this]() {
        if (m_services.audio) {
            m_services.audio->stop();
        }
    };

    ctx.setLight = [this](const weapon_behavior::LightPatternDef& pattern) {
        if (m_services.lights) {
            m_services.lights->setPattern(pattern);
        }
    };

    ctx.flashLight = [this](const weapon_behavior::LightPatternDef& pattern) {
        if (m_services.lights) {
            m_services.lights->flashPattern(pattern);
        }
    };

    ctx.emitBehaviorEvent = [this](const std::string& event) {
        if (m_behaviorController) {
            m_behaviorController->handleEvent(event);
        }
    };

    m_behaviorController = std::make_unique<WeaponBehaviorController>(
        *m_currentBehavior,
        std::move(ctx));

    m_behaviorController->initialize();

    if (m_services.debug) {
        m_services.debug->log("Equipped weapon: " + entry->name);
    }
}

const WeaponEntry *Blaster::currentWeapon() const {
    if (m_banks.empty()) {
        return nullptr;
    }
    if (m_currentBankIndex >= m_banks.size()) {
        return nullptr;
    }

    const WeaponBank &bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) {
        return nullptr;
    }
    if (m_currentWeaponIndex >= bank.weapons.size()) {
        return nullptr;
    }

    return &bank.weapons[m_currentWeaponIndex];
}

void Blaster::emitShot() const {
    if (!m_currentBehavior || !m_services.debug) {
        return;
    }

    m_services.debug->log(
        "Shot fired: " + m_currentBehavior->weapon +
        ", ammo remaining: " + std::to_string(m_currentAmmo));
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
