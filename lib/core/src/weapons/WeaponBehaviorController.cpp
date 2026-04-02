#include "core/weapons/WeaponBehaviorController.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

namespace {
std::size_t randomIndex(const std::size_t count) {
    if (count == 0) {
        return 0;
    }
    return static_cast<std::size_t>(std::rand()) % count;
}
}

WeaponBehaviorController::WeaponBehaviorController(
    const weapon_behavior::WeaponBehaviorDef& behavior,
    ShootContext context)
    : m_behavior(behavior),
      m_context(std::move(context)),
      m_currentState(behavior.initialState) {
}

void WeaponBehaviorController::initialize() {
    enterState(m_currentState);
    processPendingEvents();
}

void WeaponBehaviorController::update() {
    processScheduledEvents();
    processPendingEvents();
}

void WeaponBehaviorController::handleEvent(const std::string& event) {
    m_pendingEvents.push_back(event);
    processPendingEvents();
}

const std::string& WeaponBehaviorController::currentState() const {
    return m_currentState;
}

void WeaponBehaviorController::enterState(const std::string& stateName) {
    const auto* state = findState(stateName);
    if (!state) {
        throw std::runtime_error("WeaponBehaviorController: unknown state on enter: " + stateName);
    }

    if (m_context.debug) {
        m_context.debug->log("Behavior enter state: " + stateName);
    }

    executeActions(state->onEnter);
}

void WeaponBehaviorController::exitState() {
    const auto* state = findState(m_currentState);
    if (!state) {
        throw std::runtime_error("WeaponBehaviorController: unknown state on exit: " + m_currentState);
    }

    executeActions(state->onExit);
}

void WeaponBehaviorController::processPendingEvents() {
    if (m_isProcessingEvents) {
        return;
    }

    m_isProcessingEvents = true;

    while (!m_pendingEvents.empty()) {
        const std::string event = m_pendingEvents.front();
        m_pendingEvents.pop_front();

        const auto* state = findState(m_currentState);
        if (!state) {
            m_isProcessingEvents = false;
            throw std::runtime_error("WeaponBehaviorController: unknown current state: " + m_currentState);
        }

        const auto* transition = findTransition(*state, event);
        if (!transition) {
            continue;
        }

        if (m_context.debug) {
            m_context.debug->log(
                "Behavior transition: " + m_currentState + " --" + event + "--> " + transition->target);
        }

        exitState();
        executeActions(transition->actions);
        m_currentState = transition->target;
        enterState(m_currentState);
    }

    m_isProcessingEvents = false;
}

void WeaponBehaviorController::processScheduledEvents() {
    const std::uint64_t now = nowMs();

    std::vector<std::string> readyEvents;

    auto it = std::remove_if(
        m_scheduledEvents.begin(),
        m_scheduledEvents.end(),
        [&](const ScheduledEvent& scheduled) {
            if (scheduled.dueTimeMs <= now) {
                readyEvents.push_back(scheduled.event);
                return true;
            }
            return false;
        });

    m_scheduledEvents.erase(it, m_scheduledEvents.end());

    for (const auto& event : readyEvents) {
        m_pendingEvents.push_back(event);
    }
}

void WeaponBehaviorController::executeActions(const std::vector<ActionDef>& actions) {
    for (const auto& action : actions) {
        executeAction(action);
    }
}

void WeaponBehaviorController::executeAction(const ActionDef& action) {
    if (action.type == "play_sound") {
        if (m_context.playSound && action.sound.has_value()) {
            m_context.playSound(*action.sound, action.loop.value_or(false));
        }
        return;
    }

    if (action.type == "play_sound_random") {
        if (m_context.playRandomSound && !action.sounds.empty()) {
            m_context.playRandomSound(action.sounds, action.loop.value_or(false));
        } else if (m_context.playSound && !action.sounds.empty()) {
            m_context.playSound(
                action.sounds[randomIndex(action.sounds.size())],
                action.loop.value_or(false));
        }
        return;
    }

    if (action.type == "stop_sound" || action.type == "stop_all_sounds") {
        if (m_context.stopSound) {
            m_context.stopSound();
        }
        return;
    }

    if (action.type == "set_light") {
        if (m_context.setLight && action.pattern.has_value()) {
            m_context.setLight(*action.pattern);
        }
        return;
    }

    if (action.type == "flash_light") {
        if (m_context.flashLight && action.pattern.has_value()) {
            m_context.flashLight(*action.pattern);
        }
        return;
    }

    if (action.type == "emit_shot") {
        if (m_context.emitShot) {
            m_context.emitShot();
        }
        return;
    }

    if (action.type == "flash_muzzle") {
        if (m_context.flashMuzzle) {
            m_context.flashMuzzle();
        }
        return;
    }

    if (action.type == "consume_ammo") {
        if (m_context.ammo && action.amount.has_value()) {
            *m_context.ammo -= *action.amount;
            if (*m_context.ammo < 0) {
                *m_context.ammo = 0;
            }

            if (*m_context.ammo == 0 && m_context.emitBehaviorEvent) {
                m_context.emitBehaviorEvent("ammo_empty");
            }
        }
        return;
    }

    if (action.type == "emit_event") {
        if (action.event.has_value()) {
            m_pendingEvents.push_back(*action.event);
        }
        return;
    }

    if (action.type == "schedule_event") {
        if (action.event.has_value() && action.delayMs.has_value()) {
            scheduleEvent(*action.event, *action.delayMs);
        }
        return;
    }

    if (action.type == "run_sequence") {
        if (action.name.has_value()) {
            executeSequence(*action.name);
        }
        return;
    }

    throw std::runtime_error("WeaponBehaviorController: unknown action type: " + action.type);
}

void WeaponBehaviorController::executeSequence(const std::string& name) {
    const auto it = m_behavior.actionSequences.find(name);
    if (it == m_behavior.actionSequences.end()) {
        throw std::runtime_error("WeaponBehaviorController: unknown action sequence: " + name);
    }

    executeActions(it->second);
}

void WeaponBehaviorController::scheduleEvent(const std::string& event, const int delayMs) {
    m_scheduledEvents.push_back(ScheduledEvent{
        event,
        nowMs() + static_cast<std::uint64_t>(delayMs)
    });
}

const StateDef* WeaponBehaviorController::findState(const std::string& stateName) const {
    const auto it = m_behavior.states.find(stateName);
    if (it == m_behavior.states.end()) {
        return nullptr;
    }
    return &it->second;
}

const TransitionDef* WeaponBehaviorController::findTransition(
    const StateDef& state,
    const std::string& event) {
    for (const auto& transition : state.transitions) {
        if (transition.event == event) {
            return &transition;
        }
    }
    return nullptr;
}

std::uint64_t WeaponBehaviorController::nowMs() const {
    if (!m_context.time) {
        return 0;
    }
    return static_cast<std::uint64_t>(m_context.time->millis());
}