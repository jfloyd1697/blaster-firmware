//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H

#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace weapon_behavior {

    enum class LightPatternMode {
        Solid,
        Flash,
        Pulse,
        Sequence
    };

    struct LightStepDef {
        std::array<int, 3> color{0, 0, 0};
        int durationMs = 0;
    };

    struct LightPatternDef {
        LightPatternMode mode = LightPatternMode::Solid;
        std::array<int, 3> color{0, 0, 0};
        std::optional<int> brightness;
        std::optional<int> durationMs;
        std::optional<int> count;
        std::optional<int> intervalMs;
        std::vector<LightStepDef> steps;
    };

    struct ActionDef {
        std::string type;

        std::optional<std::string> sound;
        std::vector<std::string> sounds;
        std::optional<bool> loop;

        std::optional<LightPatternDef> pattern;

        std::optional<std::string> event;
        std::optional<std::string> name;

        std::optional<int> amount;
        std::optional<int> delayMs;
    };

    struct TransitionDef {
        std::string event;
        std::string target;
        std::vector<ActionDef> actions;
    };

    struct StateDef {
        std::vector<ActionDef> onEnter;
        std::vector<ActionDef> onExit;
        std::vector<TransitionDef> transitions;
    };

    struct WeaponBehaviorDef {
        int version = 1;
        std::string weapon;
        int magazineSize = 0;
        std::string initialState;
        std::unordered_map<std::string, std::vector<ActionDef>> actionSequences;
        std::unordered_map<std::string, StateDef> states;
    };

} // namespace weapon_behavior

#endif //BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H