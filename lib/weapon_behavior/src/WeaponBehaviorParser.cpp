#include "weapon_behavior/WeaponBehaviorParser.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace weapon_behavior {
    namespace {
        std::runtime_error parseError(const std::string &message) {
            return std::runtime_error("WeaponBehaviorParser: " + message);
        }

        std::string requireString(const json &j, const char *key) {
            if (!j.contains(key) || !j.at(key).is_string()) {
                throw parseError(std::string("missing or invalid string field '") + key + "'");
            }
            return j.at(key).get<std::string>();
        }

        std::optional<std::string> optionalString(const json &j, const char *key) {
            if (!j.contains(key)) {
                return std::nullopt;
            }
            if (!j.at(key).is_string()) {
                throw parseError(std::string("field '") + key + "' must be a string");
            }
            return j.at(key).get<std::string>();
        }

        std::optional<bool> optionalBool(const json &j, const char *key) {
            if (!j.contains(key)) {
                return std::nullopt;
            }
            if (!j.at(key).is_boolean()) {
                throw parseError(std::string("field '") + key + "' must be a boolean");
            }
            return j.at(key).get<bool>();
        }

        std::optional<int> optionalInt(const json &j, const char *key, int defaultValue = 0) {
            if (!j.contains(key)) {
                return defaultValue;
            }
            if (!j.at(key).is_number_integer()) {
                throw parseError(std::string("field '") + key + "' must be an integer");
            }
            return j.at(key).get<int>();
        }

        std::array<int, 3> parseColor(const json &j) {
            if (!j.is_array() || j.size() != 3) {
                throw parseError("light pattern color must be an array of 3 integers");
            }

            std::array<int, 3> color{};
            for (size_t i = 0; i < 3; ++i) {
                if (!j.at(i).is_number_integer()) {
                    throw parseError("light pattern color values must be integers");
                }
                color[i] = j.at(i).get<int>();
            }
            return color;
        }

        LightPatternMode parseLightPatternMode(const std::string &value) {
            if (value == "solid") return LightPatternMode::Solid;
            if (value == "flash") return LightPatternMode::Flash;
            if (value == "pulse") return LightPatternMode::Pulse;
            if (value == "sequence") return LightPatternMode::Sequence;
            throw parseError("unknown light pattern mode '" + value + "'");
        }

        LightStepDef parseLightStep(const json &j) {
            if (!j.is_object()) {
                throw parseError("light sequence step must be an object");
            }

            LightStepDef step;
            if (!j.contains("color")) {
                throw parseError("light sequence step missing 'color'");
            }
            step.color = parseColor(j.at("color"));

            if (!j.contains("durationMs") || !j.at("durationMs").is_number_integer()) {
                throw parseError("light sequence step missing or invalid 'durationMs'");
            }
            step.durationMs = j.at("durationMs").get<int>();
            return step;
        }

        LightPatternDef parseLightPattern(const json &j) {
            if (!j.is_object()) {
                throw parseError("field 'pattern' must be an object");
            }

            LightPatternDef pattern;
            pattern.mode = parseLightPatternMode(requireString(j, "mode"));

            if (j.contains("color")) {
                pattern.color = parseColor(j.at("color"));
            }

            pattern.brightness = optionalInt(j, "brightness");
            pattern.durationMs = optionalInt(j, "durationMs");
            pattern.count = optionalInt(j, "count");
            pattern.intervalMs = optionalInt(j, "intervalMs");

            if (j.contains("steps")) {
                if (!j.at("steps").is_array()) {
                    throw parseError("field 'steps' must be an array");
                }
                for (const auto &stepJson: j.at("steps")) {
                    pattern.steps.push_back(parseLightStep(stepJson));
                }
            }

            return pattern;
        }

        ActionDef parseAction(const json &j) {
            if (!j.is_object()) {
                throw parseError("action must be an object");
            }

            ActionDef action;
            action.type = requireString(j, "type");
            action.sound = optionalString(j, "sound");
            action.loop = optionalBool(j, "loop");
            action.event = optionalString(j, "event");
            action.name = optionalString(j, "name");
            action.amount = optionalInt(j, "amount");
            action.delayMs = optionalInt(j, "delayMs");

            if (j.contains("sounds")) {
                if (!j.at("sounds").is_array()) {
                    throw parseError("field 'sounds' must be an array");
                }
                for (const auto &item: j.at("sounds")) {
                    if (!item.is_string()) {
                        throw parseError("field 'sounds' must contain only strings");
                    }
                    action.sounds.push_back(item.get<std::string>());
                }
            }

            if (j.contains("pattern")) {
                action.pattern = parseLightPattern(j.at("pattern"));
            }

            return action;
        }

        std::vector<ActionDef> parseActionList(const json &j, const char *fieldName) {
            if (!j.contains(fieldName)) {
                return {};
            }
            if (!j.at(fieldName).is_array()) {
                throw parseError(std::string("field '") + fieldName + "' must be an array");
            }

            std::vector<ActionDef> result;
            for (const auto &item: j.at(fieldName)) {
                result.push_back(parseAction(item));
            }
            return result;
        }

        TransitionDef parseTransition(const json &j) {
            if (!j.is_object()) {
                throw parseError("transition must be an object");
            }

            TransitionDef t;
            t.event = requireString(j, "event");
            t.target = requireString(j, "target");
            t.actions = parseActionList(j, "actions");
            return t;
        }

        StateDef parseState(const json &j) {
            if (!j.is_object()) {
                throw parseError("state must be an object");
            }

            StateDef state;
            state.onEnter = parseActionList(j, "onEnter");
            state.onExit = parseActionList(j, "onExit");

            if (j.contains("transitions")) {
                if (!j.at("transitions").is_array()) {
                    throw parseError("field 'transitions' must be an array");
                }
                for (const auto &t: j.at("transitions")) {
                    state.transitions.push_back(parseTransition(t));
                }
            }

            return state;
        }
    } // namespace

    WeaponBehaviorDef WeaponBehaviorParser::parseFromText(const std::string &jsonText) {
        json root = json::parse(jsonText);

        if (!root.is_object()) {
            throw parseError("root JSON value must be an object");
        }

        WeaponBehaviorDef def;

        if (root.contains("version")) {
            if (!root.at("version").is_number_integer()) {
                throw parseError("field 'version' must be an integer");
            }
            def.version = root.at("version").get<int>();
        }

        def.weapon = optionalString(root, "weapon").value_or("");
        def.magazineSize = optionalInt(root, "magazineSize").value_or(0);
        def.initialState = requireString(root, "initialState");

        if (root.contains("actionSequences")) {
            if (!root.at("actionSequences").is_object()) {
                throw parseError("field 'actionSequences' must be an object");
            }

            for (auto it = root.at("actionSequences").begin(); it != root.at("actionSequences").end(); ++it) {
                if (!it.value().is_array()) {
                    throw parseError("action sequence '" + it.key() + "' must be an array");
                }

                std::vector<ActionDef> actions;
                for (const auto &actionJson: it.value()) {
                    actions.push_back(parseAction(actionJson));
                }
                def.actionSequences[it.key()] = std::move(actions);
            }
        }

        if (!root.contains("states") || !root.at("states").is_object()) {
            throw parseError("missing or invalid object field 'states'");
        }

        for (auto it = root.at("states").begin(); it != root.at("states").end(); ++it) {
            def.states[it.key()] = parseState(it.value());
        }

        return def;
    }
} // namespace weapon_behavior
