#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H

#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "core/weapons/ShootContext.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

class WeaponBehaviorController {
public:
    WeaponBehaviorController(const WeaponBehaviorDef &behavior, ShootContext context);

    void initialize();

    void update();

    void handleEvent(const std::string &event);

    [[nodiscard]] const std::string &currentState() const;

private:
    struct ScheduledEvent {
        std::string event;
        std::uint64_t dueTimeMs = 0;
    };

    void enterState(const std::string &stateName);

    void exitState();

    void processPendingEvents();

    void processScheduledEvents();

    void executeActions(const std::vector<ActionDef> &actions);

    void executeAction(const ActionDef &action);

    void executeSequence(const std::string &name);

    void scheduleEvent(const std::string &event, int delayMs);

    [[nodiscard]] const StateDef *findState(const std::string &stateName) const;

    [[nodiscard]] static const TransitionDef *findTransition(
        const StateDef &state,
        const std::string &event);

    [[nodiscard]] std::uint64_t nowMs() const;

    const WeaponBehaviorDef &m_behavior;
    ShootContext m_context;
    std::string m_currentState;
    std::deque<std::string> m_pendingEvents;
    std::vector<ScheduledEvent> m_scheduledEvents;
    bool m_isProcessingEvents = false;
};

#endif // BLASTER_FIRMWARE_WEAPONBEHAVIORCONTROLLER_H
