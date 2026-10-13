#include "core/Blaster.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <utility>

#include "core/audio/IAudioEngine.h"
#include "core/debug/IDebug.h"
#include "core/display/IStatusDisplay.h"
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
    if (m_behaviorController) {
        m_behaviorController->update();
        if (m_services.display && m_lastPublishedBehaviorState != m_behaviorController->currentStateLabel()) {
            publishDisplay();
        }
    }
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
    if (!m_services.input || !m_services.input->wasReloadPressed()) return;
    if (m_behaviorController) reloadCurrentWeapon();
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
#ifdef PLATFORM_STM32F411
    // Selection is a control event, never a filesystem operation.
    // Stop the old behavior immediately and coalesce rapid selections.
    if (m_services.audio) m_services.audio->stop();
    if (m_services.lights) m_services.lights->stopSequence();
    m_behaviorController.reset();
    m_weaponLoadPending = true;
    publishDisplay();
#else
    servicePendingWeaponLoad();
#endif
}

void Blaster::servicePendingWeaponLoad() {
#ifdef PLATFORM_STM32F411
    if (!m_weaponLoadPending) return;
    m_weaponLoadPending = false;
#endif
    // A new weapon must never inherit audio, animation, or pending events
    // from the previously equipped weapon (including failed loads).
    if (m_services.audio) {
        m_services.audio->stop();
        m_services.audio->releasePreparedSounds();
    }
    if (m_services.lights) m_services.lights->stopSequence();
    m_behaviorController.reset();
    m_currentBehavior.reset();

    const WeaponEntry* entry = currentWeapon();
    if (entry == nullptr || !m_services.textLoader) { publishDisplay(); return; }

    const std::string text = m_services.textLoader->loadText(entry->behaviorPath);
    if (text.empty()) {
        if (m_services.debug) m_services.debug->error("Blaster: failed to load " + entry->behaviorPath);
        publishDisplay();
        return;
    }

    if (entry->behaviorPath.size() < 4 || entry->behaviorPath.substr(entry->behaviorPath.size() - 4) != ".bin" ||
        !weapon_behavior::WeaponBehaviorParser::isBinaryFormat(text)) {
        if (m_services.debug) m_services.debug->error("Blaster: expected WFSMBIN1: " + entry->behaviorPath);
        publishDisplay();
        return;
    }
    m_currentBehavior = weapon_behavior::WeaponBehaviorParser::parse(text, m_services.debug.get());
    const auto validation = weapon_behavior::validateWeaponBehavior(*m_currentBehavior);
    if (!validation.ok) {
        if (m_services.debug) {
            m_services.debug->error("Blaster: invalid compiled weapon " + entry->behaviorPath);
            for (const auto& issue : validation.errors) m_services.debug->error(issue);
        }
        m_currentBehavior.reset();
        publishDisplay();
        return;
    }

    // Prepare every unique audio file for the selected weapon, including
    // random clip sets and effects. Full PCM is streamed: preloading the WAVs
    // themselves would exceed the STM32F411 RAM budget.
    if (m_services.audio) {
        std::vector<std::string> clipPaths;
        clipPaths.reserve(m_currentBehavior->clips.size());
        std::unordered_set<std::string> seen;
        for (const auto& clip : m_currentBehavior->clips) {
            if (!clip.second.path.empty() && seen.insert(clip.second.path).second) {
                clipPaths.push_back(clip.second.path);
            }
        }
        if (!m_services.audio->prepareSounds(clipPaths) && m_services.debug) {
            m_services.debug->error("Blaster: some audio clips could not be prepared for " + entry->name);
        }
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
    context.playLight = [this](const std::string& path, const LightSequencePlayback playback) {
        if (!m_services.lights || !m_services.textLoader) return;
        if (!m_services.lights->requestSequenceFile(path, *m_services.textLoader, playback) && m_services.debug) {
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
    publishDisplay();
}

void Blaster::publishDisplay() {
    if (!m_services.display) return;
    DisplayStatus status;
    status.bankCount = m_banks.size();
    status.bankIndex = m_currentBankIndex;
    if (m_currentBankIndex < m_banks.size()) {
        const auto& bank = m_banks[m_currentBankIndex];
        status.bankName = bank.name;
        status.weaponIndex = m_currentWeaponIndex;
        status.weaponCount = bank.weapons.size();
    }
    if (const auto* weapon = currentWeapon()) status.weaponName = weapon->name;
    status.weaponState = m_weaponLoadPending ? DisplayWeaponState::Loading :
        m_behaviorController ? DisplayWeaponState::Ready : DisplayWeaponState::Error;
    if (m_behaviorController) {
        status.behaviorState = m_behaviorController->currentStateLabel();
    }
    m_lastPublishedBehaviorState = status.behaviorState;
    m_services.display->present(status);
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
