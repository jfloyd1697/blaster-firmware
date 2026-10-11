#include "weapon_behavior/WeaponBehaviorValidation.h"

#include <unordered_set>

namespace weapon_behavior {
namespace {

void addError(ValidationResult& result, const std::string& message) {
    result.ok = false;
    result.errors.push_back(message);
}

bool knownAction(const std::string& type) {
    static const std::unordered_set<std::string> types{
        "play_audio", "play_audio_loop", "play_random_audio", "play_audio_effect", "play_sound_random",
        "stop_audio", "play_light", "stop_light", "start_light_sequence", "stop_light_sequence",
        "set_var", "add_var", "adjust_ammo", "set_ammo", "set_ammo_full", "emit_event",
        "schedule_event", "cancel_scheduled_events", "chance_event", "log"
    };
    return types.count(type) != 0;
}

void validateAction(const ActionDef& action, const WeaponBehaviorDef& def, ValidationResult& result, const std::string& where) {
    if (!knownAction(action.type)) addError(result, where + ": unknown action type '" + action.type + "'");
    if ((action.type == "play_audio" || action.type == "play_audio_loop") && (!action.clip || def.clips.count(*action.clip) == 0)) {
        addError(result, where + ": references unknown clip");
    }
    if (action.type == "play_random_audio") {
        if (action.clips.empty()) {
            addError(result, where + ": play_random_audio requires at least one clip");
        }
        for (const auto& clip : action.clips) {
            if (def.clips.count(clip) == 0) {
                addError(result, where + ": references unknown clip '" + clip + "'");
            }
        }
    }
    if (action.type == "play_audio_effect" && (!action.effect || def.audioEffects.count(*action.effect) == 0)) {
        addError(result, where + ": references unknown audio effect");
    }
    if (action.type == "play_sound_random" && (!action.clipSet || def.clipSets.count(*action.clipSet) == 0)) {
        addError(result, where + ": references unknown clip set");
    }
    if ((action.type == "play_light" || action.type == "start_light_sequence") && (!action.sequence || def.lightSequences.count(*action.sequence) == 0)) {
        addError(result, where + ": references unknown light sequence");
    }
}

}

ValidationResult validateWeaponBehavior(const WeaponBehaviorDef& def) {
    ValidationResult result;
    if (def.states.empty()) addError(result, "states must not be empty");
    if (def.initialState.empty() || def.states.count(def.initialState) == 0) addError(result, "initial_state does not reference a state");
    for (const auto& [id, state] : def.states) {
        for (std::size_t i = 0; i < state.onEntry.size(); ++i) validateAction(state.onEntry[i], def, result, "states." + id + ".on_entry[" + std::to_string(i) + "]");
        for (std::size_t i = 0; i < state.onExit.size(); ++i) validateAction(state.onExit[i], def, result, "states." + id + ".on_exit[" + std::to_string(i) + "]");
    }
    for (std::size_t i = 0; i < def.transitions.size(); ++i) {
        const auto& t = def.transitions[i];
        const std::string where = "transitions[" + std::to_string(i) + "]";
        if (def.states.count(t.source) == 0) addError(result, where + ": unknown source state");
        if (!t.internal && def.states.count(t.target) == 0) addError(result, where + ": unknown target state");
        if (t.trigger.empty()) addError(result, where + ": trigger must not be empty");
        for (std::size_t a = 0; a < t.actions.size(); ++a) validateAction(t.actions[a], def, result, where + ".actions[" + std::to_string(a) + "]");
    }
    for (const auto& [name, clipSet] : def.clipSets) {
        for (const auto& clip : clipSet.clips) if (def.clips.count(clip) == 0) addError(result, "clip_sets." + name + ": unknown clip '" + clip + "'");
    }
    for (const auto& [name, effect] : def.audioEffects) {
        for (const auto& clip : effect.clips) if (def.clips.count(clip) == 0) addError(result, "audio_effects." + name + ": unknown clip '" + clip + "'");
    }
    return result;
}

} // namespace weapon_behavior
