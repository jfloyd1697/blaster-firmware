#include "weapon_behavior/WeaponBehaviorParser.h"

#include <nlohmann/json.hpp>

#include "core/debug/IDebug.h"

using json = nlohmann::json;

namespace weapon_behavior {
namespace {

void logError(IDebug* debug, const std::string& message) {
    if (debug) {
        debug->error("WeaponBehaviorParser: " + message);
    }
}

std::string requireString(const json& j, const char* key, IDebug* debug) {
    if (!j.contains(key) || !j.at(key).is_string()) {
        logError(debug, std::string("missing or invalid string field '") + key + "'");
        return "";
    }
    return j.at(key).get<std::string>();
}

std::optional<std::string> optionalString(const json& j, const char* key, IDebug* debug) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    if (!j.at(key).is_string()) {
        logError(debug, std::string("field '") + key + "' must be a string");
        return std::nullopt;
    }
    return j.at(key).get<std::string>();
}

std::optional<bool> optionalBool(const json& j, const char* key, IDebug* debug) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    if (!j.at(key).is_boolean()) {
        logError(debug, std::string("field '") + key + "' must be a boolean");
        return std::nullopt;
    }
    return j.at(key).get<bool>();
}

std::optional<int> optionalInt(const json& j, const char* key, IDebug* debug) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    if (!j.at(key).is_number_integer()) {
        logError(debug, std::string("field '") + key + "' must be an integer");
        return std::nullopt;
    }
    return j.at(key).get<int>();
}

std::array<int, 3> parseColor(const json& j, IDebug* debug) {
    std::array<int, 3> color{0, 0, 0};

    if (!j.is_array() || j.size() != 3) {
        logError(debug, "light pattern color must be an array of 3 integers");
        return color;
    }

    for (std::size_t i = 0; i < 3; ++i) {
        if (!j.at(i).is_number_integer()) {
            logError(debug, "light pattern color values must be integers");
            return color;
        }
        color[i] = j.at(i).get<int>();
    }

    return color;
}

LightPatternMode parseLightPatternMode(const std::string& value, IDebug* debug) {
    if (value == "solid") return LightPatternMode::Solid;
    if (value == "flash") return LightPatternMode::Flash;
    if (value == "pulse") return LightPatternMode::Pulse;
    if (value == "sequence") return LightPatternMode::Sequence;

    logError(debug, "unknown light pattern mode '" + value + "'");
    return LightPatternMode::Solid;
}

LightStepDef parseLightStep(const json& j, IDebug* debug) {
    LightStepDef step{};

    if (!j.is_object()) {
        logError(debug, "light sequence step must be an object");
        return step;
    }

    if (!j.contains("color")) {
        logError(debug, "light sequence step missing 'color'");
    } else {
        step.color = parseColor(j.at("color"), debug);
    }

    if (!j.contains("durationMs") || !j.at("durationMs").is_number_integer()) {
        logError(debug, "light sequence step missing or invalid 'durationMs'");
    } else {
        step.durationMs = j.at("durationMs").get<int>();
    }

    return step;
}

LightPatternDef parseLightPattern(const json& j, IDebug* debug) {
    LightPatternDef pattern{};

    if (!j.is_object()) {
        logError(debug, "field 'pattern' must be an object");
        return pattern;
    }

    const std::string mode = requireString(j, "mode", debug);
    pattern.mode = parseLightPatternMode(mode, debug);

    if (j.contains("color")) {
        pattern.color = parseColor(j.at("color"), debug);
    }

    pattern.brightness = optionalInt(j, "brightness", debug);
    pattern.durationMs = optionalInt(j, "durationMs", debug);
    pattern.count = optionalInt(j, "count", debug);
    pattern.intervalMs = optionalInt(j, "intervalMs", debug);
    pattern.blockMs = optionalInt(j, "blockMs", debug);

    if (j.contains("steps")) {
        if (!j.at("steps").is_array()) {
            logError(debug, "field 'steps' must be an array");
        } else {
            for (const auto& stepJson : j.at("steps")) {
                pattern.steps.push_back(parseLightStep(stepJson, debug));
            }
        }
    }

    return pattern;
}

ActionDef parseAction(const json& j, IDebug* debug) {
    ActionDef action{};

    if (!j.is_object()) {
        logError(debug, "action must be an object");
        return action;
    }

    action.type = requireString(j, "type", debug);
    action.sound = optionalString(j, "sound", debug);
    action.loop = optionalBool(j, "loop", debug);
    action.blocking = optionalBool(j, "blocking", debug);
    action.event = optionalString(j, "event", debug);
    action.name = optionalString(j, "name", debug);
    action.amount = optionalInt(j, "amount", debug);
    action.delayMs = optionalInt(j, "delayMs", debug);
    action.blockMs = optionalInt(j, "blockMs", debug);

    if (j.contains("sounds")) {
        if (!j.at("sounds").is_array()) {
            logError(debug, "field 'sounds' must be an array");
        } else {
            for (const auto& item : j.at("sounds")) {
                if (!item.is_string()) {
                    logError(debug, "field 'sounds' must contain only strings");
                } else {
                    action.sounds.push_back(item.get<std::string>());
                }
            }
        }
    }

    if (j.contains("pattern")) {
        action.pattern = parseLightPattern(j.at("pattern"), debug);
    }

    return action;
}

std::vector<ActionDef> parseActionList(const json& j, const char* fieldName, IDebug* debug) {
    std::vector<ActionDef> result;

    if (!j.contains(fieldName)) {
        return result;
    }

    if (!j.at(fieldName).is_array()) {
        logError(debug, std::string("field '") + fieldName + "' must be an array");
        return result;
    }

    for (const auto& item : j.at(fieldName)) {
        result.push_back(parseAction(item, debug));
    }

    return result;
}

TransitionDef parseTransition(const json& j, IDebug* debug) {
    TransitionDef t{};

    if (!j.is_object()) {
        logError(debug, "transition must be an object");
        return t;
    }

    t.event = requireString(j, "event", debug);
    t.target = requireString(j, "target", debug);
    t.actions = parseActionList(j, "actions", debug);
    return t;
}

StateDef parseState(const json& j, IDebug* debug) {
    StateDef state{};

    if (!j.is_object()) {
        logError(debug, "state must be an object");
        return state;
    }

    state.onEnter = parseActionList(j, "onEnter", debug);
    state.onExit = parseActionList(j, "onExit", debug);

    if (j.contains("transitions")) {
        if (!j.at("transitions").is_array()) {
            logError(debug, "field 'transitions' must be an array");
        } else {
            for (const auto& t : j.at("transitions")) {
                state.transitions.push_back(parseTransition(t, debug));
            }
        }
    }

    return state;
}

} // namespace

WeaponBehaviorDef WeaponBehaviorParser::parseFromText(const std::string& jsonText, IDebug* debug) {
    WeaponBehaviorDef def{};

    json root = json::parse(jsonText, nullptr, false);

    if (root.is_discarded()) {
        logError(debug, "json parse failed");
        return def;
    }

    if (!root.is_object()) {
        logError(debug, "root JSON value must be an object");
        return def;
    }

    if (root.contains("version")) {
        if (!root.at("version").is_number_integer()) {
            logError(debug, "field 'version' must be an integer");
        } else {
            def.version = root.at("version").get<int>();
        }
    }

    def.weapon = optionalString(root, "weapon", debug).value_or("");

    if (root.contains("magazineSize")) {
        if (!root.at("magazineSize").is_number_integer()) {
            logError(debug, "field 'magazineSize' must be an integer");
        } else {
            def.magazineSize = root.at("magazineSize").get<int>();
        }
    }

    def.initialState = requireString(root, "initialState", debug);

    if (root.contains("actionSequences")) {
        if (!root.at("actionSequences").is_object()) {
            logError(debug, "field 'actionSequences' must be an object");
        } else {
            for (auto it = root.at("actionSequences").begin(); it != root.at("actionSequences").end(); ++it) {
                if (!it.value().is_array()) {
                    logError(debug, "action sequence '" + it.key() + "' must be an array");
                    continue;
                }

                std::vector<ActionDef> actions;
                for (const auto& actionJson : it.value()) {
                    actions.push_back(parseAction(actionJson, debug));
                }
                def.actionSequences[it.key()] = std::move(actions);
            }
        }
    }

    if (!root.contains("states") || !root.at("states").is_object()) {
        logError(debug, "missing or invalid object field 'states'");
        return def;
    }

    for (auto it = root.at("states").begin(); it != root.at("states").end(); ++it) {
        def.states[it.key()] = parseState(it.value(), debug);
    }

    return def;
}

} // namespace weapon_behavior