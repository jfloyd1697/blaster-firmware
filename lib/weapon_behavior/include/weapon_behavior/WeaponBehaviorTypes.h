#ifndef BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H
#define BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace weapon_behavior {

using RuntimeValue = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

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
    std::optional<int> blockMs;
    std::vector<LightStepDef> steps;
};

struct VariableComparisonDef {
    std::string name;
    std::optional<RuntimeValue> value;
    std::optional<std::string> valueFromVar;
};

struct GuardDef {
    std::vector<GuardDef> all;
    std::vector<GuardDef> any;
    std::optional<bool> triggerPressed;
    std::optional<VariableComparisonDef> varEq;
    std::optional<VariableComparisonDef> varGt;
    std::optional<VariableComparisonDef> varGte;
    std::optional<VariableComparisonDef> varLt;
    std::optional<VariableComparisonDef> varLte;
};

struct ActionDef {
    std::string type;
    std::optional<std::string> clip;
    std::vector<std::string> clips;
    std::optional<std::string> mode;
    std::optional<std::string> interrupt;
    std::optional<std::string> effect;
    std::optional<std::string> clipSet;
    std::optional<std::string> sequence;
    std::optional<std::string> name;
    std::optional<RuntimeValue> value;
    std::optional<std::string> valueFromVar;
    std::optional<int> delta;
    std::optional<std::string> event;
    std::optional<int> delayMs;
    std::optional<std::string> delayMsFromVar;
    std::optional<double> chance;
    std::optional<std::string> message;
};

struct ClipDef {
    std::string path;
    bool preload = true;
};

struct ClipSetDef {
    std::vector<std::string> clips;
    std::string mode = "random";
};

struct AudioEffectDef {
    std::vector<std::string> clips;
    std::string mode = "once";
    std::string interrupt = "interrupt";
    bool loop = false;
    double gain = 1.0;
};

struct LightSequenceDef {
    std::string path;
    bool preload = true;
};

struct StateDef {
    std::string id;
    std::string label;
    std::vector<ActionDef> onEntry;
    std::vector<ActionDef> onExit;
};

struct TransitionDef {
    std::string id;
    std::string source;
    std::string trigger;
    std::string target;
    std::optional<GuardDef> guard;
    std::vector<ActionDef> actions;
    bool internal = false;
};

struct WeaponBehaviorDef {
    int formatVersion = 1;
    std::string initialState = "ready";
    std::unordered_map<std::string, RuntimeValue> variables;
    std::unordered_map<std::string, StateDef> states;
    std::vector<TransitionDef> transitions;
    std::unordered_map<std::string, ClipDef> clips;
    std::unordered_map<std::string, ClipSetDef> clipSets;
    std::unordered_map<std::string, AudioEffectDef> audioEffects;
    std::unordered_map<std::string, LightSequenceDef> lightSequences;
};

} // namespace weapon_behavior

#endif // BLASTER_FIRMWARE_WEAPONBEHAVIORTYPES_H
