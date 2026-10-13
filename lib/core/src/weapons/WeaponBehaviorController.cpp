#include "core/weapons/WeaponBehaviorController.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "core/debug/IDebug.h"
#include "core/time/ITime.h"

namespace {

using weapon_behavior::RuntimeValue;

std::size_t randomIndex(const std::size_t count) {
    return count == 0 ? 0 : static_cast<std::size_t>(std::rand()) % count;
}

bool asNumber(const RuntimeValue& value, double& out) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        out = static_cast<double>(*integer);
        return true;
    }
    if (const auto* floating = std::get_if<double>(&value)) {
        out = *floating;
        return true;
    }
    return false;
}

bool valuesEqual(const RuntimeValue& left, const RuntimeValue& right) {
    double lhs = 0.0;
    double rhs = 0.0;
    if (asNumber(left, lhs) && asNumber(right, rhs)) return lhs == rhs;
    return left == right;
}

bool compareValues(const RuntimeValue& left, const RuntimeValue& right, const int relation) {
    if (relation == 0) return valuesEqual(left, right);
    double lhs = 0.0;
    double rhs = 0.0;
    if (!asNumber(left, lhs) || !asNumber(right, rhs)) return false;
    if (relation == 1) return lhs > rhs;
    if (relation == 2) return lhs >= rhs;
    if (relation == -1) return lhs < rhs;
    if (relation == -2) return lhs <= rhs;
    return false;
}

int valueAsInt(const RuntimeValue& value, const int fallback = 0) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) return static_cast<int>(*integer);
    if (const auto* floating = std::get_if<double>(&value)) return static_cast<int>(*floating);
    return fallback;
}

RuntimeValue addValues(const RuntimeValue& current, const RuntimeValue& delta) {
    if (const auto* leftInt = std::get_if<std::int64_t>(&current)) {
        if (const auto* rightInt = std::get_if<std::int64_t>(&delta)) return *leftInt + *rightInt;
    }
    double lhs = 0.0;
    double rhs = 0.0;
    if (asNumber(current, lhs) && asNumber(delta, rhs)) return lhs + rhs;
    return current;
}

} // namespace

WeaponBehaviorController::WeaponBehaviorController(
    const weapon_behavior::WeaponBehaviorDef& behavior,
    ShootContext context)
    : m_behavior(behavior),
      m_context(std::move(context)),
      m_currentState(behavior.initialState),
      m_variables(behavior.variables) {
    m_variables["trigger_down"] = false;
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
    applyInputEvent(event);
    m_pendingEvents.push_back(event);
    processPendingEvents();
}

const std::string& WeaponBehaviorController::currentState() const {
    return m_currentState;
}

const std::string& WeaponBehaviorController::currentStateLabel() const {
    const auto* state = findState(m_currentState);
    return state != nullptr && !state->label.empty() ? state->label : m_currentState;
}

const std::unordered_map<std::string, RuntimeValue>& WeaponBehaviorController::variables() const {
    return m_variables;
}

void WeaponBehaviorController::applyInputEvent(const std::string& event) {
    if (event == "trigger_pressed" || event == "trigger_held" || event == "trigger_released") {
        m_variables["trigger_state"] = event;
    }
    if (event == "trigger_pressed") {
        m_triggerPressed = true;
        m_variables["trigger_down"] = true;
    } else if (event == "trigger_released") {
        m_triggerPressed = false;
        m_variables["trigger_down"] = false;
    }
}

void WeaponBehaviorController::enterState(const std::string& stateName) {
    const auto* state = findState(stateName);
    if (state == nullptr) {
        if (m_context.debug) m_context.debug->error("WeaponBehaviorController: unknown state on enter: " + stateName);
        return;
    }
    if (m_context.debug) m_context.debug->log("Behavior enter state: " + stateName);
    executeActions(state->onEntry);
}

void WeaponBehaviorController::exitState() {
    const auto* state = findState(m_currentState);
    if (state == nullptr) return;
    executeActions(state->onExit);
}

void WeaponBehaviorController::processPendingEvents() {
    if (m_isProcessingEvents) return;
    m_isProcessingEvents = true;
    while (!m_pendingEvents.empty()) {
        const std::string event = m_pendingEvents.front();
        m_pendingEvents.pop_front();
        const auto* transition = findTransition(event);
        if (transition == nullptr) continue;
        // Held-trigger self transitions can occur every loop iteration. Printing
        // them at USB serial speed starves audio/LED servicing on the MCU.
        const bool repeatedHold = (event == "trigger_held" || event == "trigger_tick") &&
            (transition->internal || transition->target == m_currentState);
        if (m_context.debug && !repeatedHold) {
            m_context.debug->log("Behavior transition: " + m_currentState + " --" + event + "--> " + transition->target);
        }
        applyTransition(*transition);
    }
    m_isProcessingEvents = false;
}

void WeaponBehaviorController::processScheduledEvents() {
    const std::uint64_t now = nowMs();
    auto it = std::remove_if(m_scheduledEvents.begin(), m_scheduledEvents.end(), [&](const ScheduledEvent& item) {
        if (item.dueTimeMs > now) return false;
        m_pendingEvents.push_back(item.event);
        return true;
    });
    m_scheduledEvents.erase(it, m_scheduledEvents.end());
}

void WeaponBehaviorController::applyTransition(const weapon_behavior::TransitionDef& transition) {
    if (transition.internal) {
        executeActions(transition.actions);
        return;
    }
    exitState();
    executeActions(transition.actions);
    m_currentState = transition.target;
    enterState(m_currentState);
}

void WeaponBehaviorController::executeActions(const std::vector<weapon_behavior::ActionDef>& actions) {
    for (const auto& action : actions) executeAction(action);
}

void WeaponBehaviorController::executeAction(const weapon_behavior::ActionDef& action) {
    if (action.type == "play_audio" || action.type == "play_audio_loop") {
        if (action.clip && m_context.playAudio) {
            const bool loop = action.type == "play_audio_loop" || action.mode.value_or("once") == "loop";
            m_context.playAudio(resolveClipPath(*action.clip), loop, action.interrupt.value_or("interrupt"));
        }
        return;
    }
    if (action.type == "play_random_audio") {
        if (!action.clips.empty() && m_context.playAudio) {
            const auto& clip = action.clips[randomIndex(action.clips.size())];
            m_context.playAudio(resolveClipPath(clip), false, action.interrupt.value_or("interrupt"));
        }
        return;
    }
    if (action.type == "play_audio_effect") {
        if (!action.effect || !m_context.playAudio) return;
        const auto it = m_behavior.audioEffects.find(*action.effect);
        if (it == m_behavior.audioEffects.end() || it->second.clips.empty()) return;
        const auto& effect = it->second;
        const bool loop = effect.loop || effect.mode == "loop";
        m_context.playAudio(resolveClipPath(effect.clips.front()), loop, effect.interrupt);
        return;
    }
    if (action.type == "play_sound_random") {
        if (!action.clipSet || !m_context.playAudio) return;
        const auto it = m_behavior.clipSets.find(*action.clipSet);
        if (it == m_behavior.clipSets.end() || it->second.clips.empty()) return;
        const auto& clipSet = it->second;
        std::size_t index = 0;
        if (clipSet.mode == "sequence") {
            auto state = m_clipSetIndices.find(*action.clipSet);
            index = state == m_clipSetIndices.end() ? clipSet.clips.size() : state->second;
            index = index == 0 ? clipSet.clips.size() - 1 : index - 1;
            m_clipSetIndices[*action.clipSet] = index;
        } else {
            index = randomIndex(clipSet.clips.size());
        }
        m_context.playAudio(resolveClipPath(clipSet.clips[index]), false, action.interrupt.value_or("interrupt"));
        return;
    }
    if (action.type == "stop_audio") {
        if (m_context.stopAudio) m_context.stopAudio();
        return;
    }
    if (action.type == "play_light" || action.type == "start_light_sequence") {
        if (action.sequence && m_context.playLight) {
            const bool loop = action.type == "start_light_sequence" || action.mode.value_or("once") == "loop";
            m_context.playLight(resolveLightPath(*action.sequence), loop);
        }
        return;
    }
    if (action.type == "stop_light" || action.type == "stop_light_sequence") {
        if (m_context.stopLight) m_context.stopLight();
        return;
    }
    if (action.type == "set_var") {
        if (!action.name) return;
        if (action.valueFromVar) {
            const auto* source = findVariable(*action.valueFromVar);
            if (source) m_variables[*action.name] = *source;
        } else if (action.value) {
            m_variables[*action.name] = *action.value;
        }
        return;
    }
    if (action.type == "add_var") {
        if (!action.name || !action.value) return;
        const auto* current = findVariable(*action.name);
        if (current) m_variables[*action.name] = addValues(*current, *action.value);
        return;
    }
    if (action.type == "adjust_ammo") {
        const RuntimeValue delta = static_cast<std::int64_t>(action.delta.value_or(0));
        const auto* current = findVariable("ammo");
        if (current) m_variables["ammo"] = addValues(*current, delta);
        return;
    }
    if (action.type == "set_ammo") {
        if (action.value) m_variables["ammo"] = *action.value;
        return;
    }
    if (action.type == "set_ammo_full") {
        const auto* capacity = findVariable("mag_capacity");
        if (capacity) m_variables["ammo"] = *capacity;
        return;
    }
    if (action.type == "emit_event") {
        if (action.event) m_pendingEvents.push_back(*action.event);
        return;
    }
    if (action.type == "schedule_event") {
        if (action.event) scheduleEvent(*action.event, resolveDelayMs(action));
        return;
    }
    if (action.type == "cancel_scheduled_events") {
        if (action.event) cancelScheduledEvents(*action.event);
        return;
    }
    if (action.type == "chance_event") {
        const double chance = action.chance.value_or(0.0);
        const double sample = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        if (action.event && sample <= chance) m_pendingEvents.push_back(*action.event);
        return;
    }
    if (action.type == "log") {
        if (m_context.log) m_context.log(action.message.value_or(""));
        return;
    }
    if (m_context.debug) m_context.debug->error("WeaponBehaviorController: unsupported action: " + action.type);
}

void WeaponBehaviorController::scheduleEvent(const std::string& event, const int delayMs) {
    m_scheduledEvents.push_back(ScheduledEvent{event, nowMs() + static_cast<std::uint64_t>(std::max(delayMs, 0))});
}

void WeaponBehaviorController::cancelScheduledEvents(const std::string& event) {
    m_scheduledEvents.erase(std::remove_if(m_scheduledEvents.begin(), m_scheduledEvents.end(), [&](const ScheduledEvent& item) {
        return item.event == event;
    }), m_scheduledEvents.end());
}

const weapon_behavior::StateDef* WeaponBehaviorController::findState(const std::string& stateName) const {
    const auto it = m_behavior.states.find(stateName);
    return it == m_behavior.states.end() ? nullptr : &it->second;
}

const weapon_behavior::TransitionDef* WeaponBehaviorController::findTransition(const std::string& event) const {
    for (const auto& transition : m_behavior.transitions) {
        if (transition.source != m_currentState || transition.trigger != event) continue;
        if (transition.guard && !guardAllows(*transition.guard)) continue;
        return &transition;
    }
    return nullptr;
}

bool WeaponBehaviorController::guardAllows(const weapon_behavior::GuardDef& guard) const {
    for (const auto& item : guard.all) if (!guardAllows(item)) return false;
    if (!guard.any.empty()) {
        bool matched = false;
        for (const auto& item : guard.any) matched = matched || guardAllows(item);
        if (!matched) return false;
    }
    if (guard.triggerPressed && *guard.triggerPressed != m_triggerPressed) return false;
    return comparisonAllows(guard.varEq, 0)
        && comparisonAllows(guard.varGt, 1)
        && comparisonAllows(guard.varGte, 2)
        && comparisonAllows(guard.varLt, -1)
        && comparisonAllows(guard.varLte, -2);
}

bool WeaponBehaviorController::comparisonAllows(
    const std::optional<weapon_behavior::VariableComparisonDef>& comparison,
    const int relation) const {
    if (!comparison) return true;
    const auto* left = findVariable(comparison->name);
    if (left == nullptr) return false;
    const RuntimeValue* right = nullptr;
    if (comparison->valueFromVar) right = findVariable(*comparison->valueFromVar);
    else if (comparison->value) right = &*comparison->value;
    if (right == nullptr) return false;
    return compareValues(*left, *right, relation);
}

const RuntimeValue* WeaponBehaviorController::findVariable(const std::string& name) const {
    const auto it = m_variables.find(name);
    return it == m_variables.end() ? nullptr : &it->second;
}

int WeaponBehaviorController::resolveDelayMs(const weapon_behavior::ActionDef& action) const {
    if (action.delayMsFromVar) {
        const auto* value = findVariable(*action.delayMsFromVar);
        return value == nullptr ? 0 : std::max(valueAsInt(*value), 0);
    }
    return std::max(action.delayMs.value_or(0), 0);
}

std::string WeaponBehaviorController::resolveClipPath(const std::string& clip) const {
    const auto it = m_behavior.clips.find(clip);
    return it == m_behavior.clips.end() ? clip : it->second.path;
}

std::string WeaponBehaviorController::resolveLightPath(const std::string& sequence) const {
    const auto it = m_behavior.lightSequences.find(sequence);
    return it == m_behavior.lightSequences.end() ? sequence : it->second.path;
}

std::uint64_t WeaponBehaviorController::nowMs() const {
    return m_context.time == nullptr ? 0 : m_context.time->millis();
}
