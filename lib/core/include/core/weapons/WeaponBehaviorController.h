#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H

#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/weapons/ShootContext.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class WeaponBehaviorController {
public:
    WeaponBehaviorController(const weapon_behavior::WeaponBehaviorDef& behavior, ShootContext context);

    void initialize();
    void update();
    void handleEvent(const std::string& event);

    [[nodiscard]] const std::string& currentState() const;
    [[nodiscard]] const std::unordered_map<std::string, weapon_behavior::RuntimeValue>& variables() const;

private:
    struct ScheduledEvent {
        std::string event;
        std::uint64_t dueTimeMs = 0;
    };

    void applyInputEvent(const std::string& event);
    void enterState(const std::string& stateName);
    void exitState();
    void processPendingEvents();
    void processScheduledEvents();
    void applyTransition(const weapon_behavior::TransitionDef& transition);

    void executeActions(const std::vector<weapon_behavior::ActionDef>& actions);
    void executeAction(const weapon_behavior::ActionDef& action);
    void scheduleEvent(const std::string& event, int delayMs);
    void cancelScheduledEvents(const std::string& event);

    [[nodiscard]] const weapon_behavior::StateDef* findState(const std::string& stateName) const;
    [[nodiscard]] const weapon_behavior::TransitionDef* findTransition(const std::string& event) const;
    [[nodiscard]] bool guardAllows(const weapon_behavior::GuardDef& guard) const;
    [[nodiscard]] bool comparisonAllows(
        const std::optional<weapon_behavior::VariableComparisonDef>& comparison,
        int relation) const;
    [[nodiscard]] const weapon_behavior::RuntimeValue* findVariable(const std::string& name) const;
    [[nodiscard]] int resolveDelayMs(const weapon_behavior::ActionDef& action) const;
    [[nodiscard]] std::string resolveClipPath(const std::string& clip) const;
    [[nodiscard]] std::string resolveLightPath(const std::string& sequence) const;
    [[nodiscard]] std::uint64_t nowMs() const;

    const weapon_behavior::WeaponBehaviorDef& m_behavior;
    ShootContext m_context;
    std::string m_currentState;
    std::unordered_map<std::string, weapon_behavior::RuntimeValue> m_variables;
    std::unordered_map<std::string, std::size_t> m_clipSetIndices;
    std::deque<std::string> m_pendingEvents;
    std::vector<ScheduledEvent> m_scheduledEvents;
    bool m_triggerPressed = false;
    bool m_isProcessingEvents = false;
};

#endif // BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H
