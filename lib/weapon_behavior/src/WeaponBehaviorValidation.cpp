#include "weapon_behavior/WeaponBehaviorValidation.h"

#include <unordered_set>

namespace weapon_behavior {
    namespace {
        void addError(ValidationResult &result, const std::string &error) {
            result.ok = false;
            result.errors.push_back(error);
        }

        bool isKnownActionType(const std::string &type) {
            static const std::unordered_set<std::string> known{
                "play_sound",
                "play_sound_random",
                "stop_sound",
                "stop_all_sounds",
                "set_light",
                "flash_light",
                "flash_muzzle",
                "consume_ammo",
                "reload_complete",
                "emit_event",
                "run_sequence",
                "emit_event",
                "emit_shot",
                "schedule_event"
            };
            return known.count(type) > 0;
        }

        bool hasLightModeFields(const LightPatternDef &pattern) {
            switch (pattern.mode) {
                case LightPatternMode::Solid:
                    return true;
                case LightPatternMode::Flash:
                    return pattern.durationMs.has_value();
                case LightPatternMode::Pulse:
                    return pattern.intervalMs.has_value();
                case LightPatternMode::Sequence:
                    return !pattern.steps.empty();
            }
            return false;
        }

        void validateAction(
            const ActionDef &action,
            const WeaponBehaviorDef &def,
            ValidationResult &result,
            const std::string &context) {
            if (!isKnownActionType(action.type)) {
                addError(result, context + ": unknown action type '" + action.type + "'");
                return;
            }

            if (action.type == "play_sound") {
                if (!action.sound.has_value() || action.sound->empty()) {
                    addError(result, context + ": play_sound requires 'sound'");
                }
            }

            if (action.type == "play_sound_random") {
                if (action.sounds.empty()) {
                    addError(result, context + ": play_sound_random requires non-empty 'sounds'");
                }
            }

            // if (action.type == "stop_sound") {
            //     if (!action.sound.has_value() || action.sound->empty()) {
            //         addError(result, context + ": stop_sound requires 'sound'");
            //     }
            // }

            if (action.type == "schedule_event") {
                if (!action.event.has_value() || action.event->empty()) {
                    addError(result, context + ": schedule_event requires 'event'");
                }
                if (!action.delayMs.has_value()) {
                    addError(result, context + ": schedule_event requires 'delayMs'");
                } else if (*action.delayMs < 0) {
                    addError(result, context + ": schedule_event requires delayMs >= 0");
                }
            }

            if (action.type == "set_light" || action.type == "flash_light") {
                if (!action.pattern.has_value()) {
                    addError(result, context + ": " + action.type + " requires 'pattern'");
                } else if (!hasLightModeFields(*action.pattern)) {
                    addError(result, context + ": invalid light pattern for " + action.type);
                }
            }

            if (action.type == "consume_ammo") {
                if (!action.amount.has_value()) {
                    addError(result, context + ": consume_ammo requires 'amount'");
                }
            }

            if (action.type == "emit_event") {
                if (!action.event.has_value() || action.event->empty()) {
                    addError(result, context + ": emit_event requires 'event'");
                }
            }

            if (action.type == "run_sequence") {
                if (!action.name.has_value() || action.name->empty()) {
                    addError(result, context + ": run_sequence requires 'name'");
                } else if (def.actionSequences.find(*action.name) == def.actionSequences.end()) {
                    addError(result, context + ": sequence '" + *action.name + "' does not exist");
                }
            }
        }
    } // namespace

    ValidationResult validateWeaponBehavior(const WeaponBehaviorDef &def) {
        ValidationResult result;

        if (def.initialState.empty()) {
            addError(result, "initialState must not be empty");
        } else if (def.states.find(def.initialState) == def.states.end()) {
            addError(result, "initialState '" + def.initialState + "' does not exist in states");
        }

        if (def.states.empty()) {
            addError(result, "states must not be empty");
        }

        for (const auto &[sequenceName, actions]: def.actionSequences) {
            for (size_t i = 0; i < actions.size(); ++i) {
                validateAction(
                    actions[i],
                    def,
                    result,
                    "actionSequences." + sequenceName + "[" + std::to_string(i) + "]");
            }
        }

        if (def.magazineSize <= 0) {
            addError(result, "magazineSize must be > 0");
        }

        for (const auto &[stateName, state]: def.states) {
            for (size_t i = 0; i < state.onEnter.size(); ++i) {
                validateAction(
                    state.onEnter[i],
                    def,
                    result,
                    "states." + stateName + ".onEnter[" + std::to_string(i) + "]");
            }

            for (size_t i = 0; i < state.onExit.size(); ++i) {
                validateAction(
                    state.onExit[i],
                    def,
                    result,
                    "states." + stateName + ".onExit[" + std::to_string(i) + "]");
            }

            for (size_t i = 0; i < state.transitions.size(); ++i) {
                const auto &t = state.transitions[i];
                const std::string base = "states." + stateName + ".transitions[" + std::to_string(i) + "]";

                if (t.event.empty()) {
                    addError(result, base + ": event must not be empty");
                }

                if (t.target.empty()) {
                    addError(result, base + ": target must not be empty");
                } else if (def.states.find(t.target) == def.states.end()) {
                    addError(result, base + ": target state '" + t.target + "' does not exist");
                }

                for (size_t a = 0; a < t.actions.size(); ++a) {
                    validateAction(
                        t.actions[a],
                        def,
                        result,
                        base + ".actions[" + std::to_string(a) + "]");
                }
            }
        }

        return result;
    }
} // namespace weapon_behavior
