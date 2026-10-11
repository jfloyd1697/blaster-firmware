#include "core/Blaster.h"

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
#include "weapon_behavior/WeaponBehaviorValidation.h"

Blaster::Blaster(PlatformServices& services, std::vector<WeaponBank> banks)
    : m_services(services), m_banks(std::move(banks)) {
    equipCurrentWeapon();
}

Blaster::~Blaster() = default;

bool Blaster::update() {
    if (m_banks.empty()) return false;
    handleWeaponSelectionInput();
    handleReloadInput();
    handleTriggerInput();
    if (m_behaviorController) m_behaviorController->update();
    return !shouldQuit();
}

void Blaster::handleWeaponSelectionInput() {
    if (!m_services.input) return;
    if (m_services.input->wasNextLongPressed()) { selectNextBank(); return; }
    if (m_services.input->wasPrevLongPressed()) { selectPreviousBank(); return; }
    if (m_services.input->wasNextShortPressed()) selectNextWeapon();
    if (m_services.input->wasPrevShortPressed()) selectPreviousWeapon();
}

void Blaster::handleReloadInput() {
    if (m_services.input && m_behaviorController && m_services.input->wasReloadPressed()) reloadCurrentWeapon();
}

void Blaster::handleTriggerInput() {
    if (!m_services.input || !m_behaviorController) return;
    if (m_services.input->wasTriggerPressed()) m_behaviorController->handleEvent("trigger_pressed");
    if (m_services.input->wasTriggerReleased()) m_behaviorController->handleEvent("trigger_released");
    if (m_services.input->isTriggerHeld()) m_behaviorController->handleEvent("trigger_held");
}

void Blaster::selectNextWeapon() {
    if (m_banks.empty()) return;
    const auto& bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) return;
    m_currentWeaponIndex = (m_currentWeaponIndex + 1) % bank.weapons.size();
    equipCurrentWeapon();
}

void Blaster::selectPreviousWeapon() {
    if (m_banks.empty()) return;
    const auto& bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty()) return;
    m_currentWeaponIndex = m_currentWeaponIndex == 0 ? bank.weapons.size() - 1 : m_currentWeaponIndex - 1;
    equipCurrentWeapon();
}

void Blaster::reloadCurrentWeapon() {
    if (m_behaviorController) m_behaviorController->handleEvent("reload_pressed");
}

void Blaster::selectNextBank() {
    if (m_banks.empty()) return;
    m_currentBankIndex = (m_currentBankIndex + 1) % m_banks.size();
    m_currentWeaponIndex = 0;
    equipCurrentWeapon();
}

void Blaster::selectPreviousBank() {
    if (m_banks.empty()) return;
    m_currentBankIndex = m_currentBankIndex == 0 ? m_banks.size() - 1 : m_currentBankIndex - 1;
    m_currentWeaponIndex = 0;
    equipCurrentWeapon();
}

void Blaster::equipCurrentWeapon() {
    const WeaponEntry* entry = currentWeapon();
    if (entry == nullptr || !m_services.textLoader) {
        m_currentBehavior.reset();
        m_behaviorController.reset();
        return;
    }

    const std::string text = m_services.textLoader->loadText(entry->behaviorPath);
    if (text.empty()) {
        if (m_services.debug) m_services.debug->error("Blaster: failed to load " + entry->behaviorPath);
        return;
    }

    if (entry->behaviorPath.size() < 4 || entry->behaviorPath.substr(entry->behaviorPath.size() - 4) != ".bin" ||
        !weapon_behavior::WeaponBehaviorParser::isBinaryFormat(text)) {
        if (m_services.debug) m_services.debug->error("Blaster: expected WFSMBIN1: " + entry->behaviorPath);
        m_currentBehavior.reset();
        m_behaviorController.reset();
        return;
    }
    // Dispose the previous controller before replacing the active behavior.
    m_behaviorController.reset();
    m_currentBehavior = weapon_behavior::WeaponBehaviorParser::parse(text, m_services.debug.get());
    const auto validation = weapon_behavior::validateWeaponBehavior(*m_currentBehavior);
    if (!validation.ok) {
        if (m_services.debug) {
            m_services.debug->error("Blaster: invalid compiled weapon " + entry->behaviorPath);
            for (const auto& issue : validation.errors) m_services.debug->error(issue);
        }
        m_currentBehavior.reset();
        return;
    }

    ShootContext context;
    context.time = m_services.time.get();
    context.audio = m_services.audio.get();
    context.debug = m_services.debug.get();
    context.playAudio = [this](const std::string& path, const bool loop, const std::string& interrupt) {
        if (!m_services.audio) return;
        if (interrupt == "interrupt" || interrupt == "interrupt_all") m_services.audio->stop();
        m_services.audio->playSound(path, loop, false);
    };
    context.stopAudio = [this]() {
        if (m_services.audio) m_services.audio->stop();
    };
    context.playLight = [this](const std::string& path, const bool loop) {
        if (!m_services.lights || !m_services.textLoader) return;
        if (!m_services.lights->playSequenceFile(path, *m_services.textLoader, loop) && m_services.debug) {
            m_services.debug->error("Blaster: invalid compiled light sequence: " + path);
        }
    };
    context.stopLight = [this]() {
        if (m_services.lights) m_services.lights->stopSequence();
    };
    context.log = [this](const std::string& message) {
        if (m_services.debug) m_services.debug->log(message);
    };

    m_behaviorController = std::make_unique<WeaponBehaviorController>(*m_currentBehavior, std::move(context));
    m_behaviorController->initialize();
    m_behaviorController->handleEvent("on_equip");
    if (m_services.debug) m_services.debug->log("Equipped weapon: " + entry->name);
}

const WeaponEntry* Blaster::currentWeapon() const {
    if (m_banks.empty() || m_currentBankIndex >= m_banks.size()) return nullptr;
    const auto& bank = m_banks[m_currentBankIndex];
    if (bank.weapons.empty() || m_currentWeaponIndex >= bank.weapons.size()) return nullptr;
    return &bank.weapons[m_currentWeaponIndex];
}

bool Blaster::shouldQuit() const {
    return m_services.input && m_services.input->wasQuitPressed();
}
