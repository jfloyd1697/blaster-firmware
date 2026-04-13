#include "weapon_behavior/EventWeaponController.h"


void EventWeaponController::reloadCurrentWeapon() {
    m_eventHandler.handleEvent("reload_complete");
}


EventWeaponController::EventWeaponController(
    IAudioEngine *audioEngine,
    WeaponBehaviorDef *behaviorDef) : audio(audioEngine) {
    m_behaviorDef = behaviorDef;

    const ShootContext ctx = {
        .time = nullptr,
        .debug = nullptr,
        .profile = nullptr,
        .ammo = nullptr,
        .playSound = [this](const std::string &path, bool loop) {
            if (audio) {
                audio->playSound(path, loop);
            }
        },
        .playRandomSound = [this](const std::vector<std::string> &sounds,
                                  bool loop) {
            if (audio && !sounds.empty()) {
                const std::size_t index =
                        static_cast<std::size_t>(std::rand()) % sounds.size();
                audio->playSound(sounds[index], loop);
            }
        },
        .stopSound = [this]() {
            if (audio) {
                audio->stop();
            }
        },
        .emitBehaviorEvent = [this](const std::string &event) {
            m_behaviorController->handleEvent(event);
        }
    };

    m_behaviorController = std::make_unique<WeaponBehaviorEventHandler>(behaviorDef, &ctx);
    m_behaviorController->initialize();
}

void EventWeaponController::equipWeapon(const WeaponProfile &profile) {
    if (!m_currentProfile) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no weapon available");
        }
        m_behaviorDef.reset();
        m_behaviorController.reset();
        return;
    }

    if (!m_services.loader) {
        if (m_services.debug) {
            m_services.debug->error("Blaster::equipCurrentWeapon: no loader available");
        }
        m_behaviorDef.reset();
        m_behaviorController.reset();
        return;
    }

    try {
        m_behaviorDef = loadWeaponBehavior(*m_services.loader, m_currentProfile->behaviorPath);
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
    ctx.debug = m_services.debug.get();
    ctx.profile = m_currentProfile;
    ctx.ammo = &m_currentAmmo;

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

    ctx.emitBehaviorEvent = [this](const std::string &event) {
        if (m_behaviorController) {
            m_behaviorController->handleEvent(event);
        }
    };

    m_behaviorController = std::make_unique<WeaponBehaviorEventHandler>(
        m_behaviorDef,
        std::move(ctx));

    m_behaviorController->initialize();
}


void EventWeaponController::reloadWeapon() {
    m_behaviorController->handleEvent("reload");
    m_behaviorController->handleEvent("reload_complete");
}

void EventWeaponController::onEmpty() {
    IWeaponController::onEmpty();
}

void EventWeaponController::onTriggerPressed() {
    m_behaviorController->handleEvent("trigger_pressed");
}

void EventWeaponController::onTriggerHeld() {
    m_behaviorController->handleEvent("trigger_held");
}

void EventWeaponController::onTriggerReleased() {
    m_behaviorController->handleEvent("trigger_released");
}
