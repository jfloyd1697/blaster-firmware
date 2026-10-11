#include "weapon_behavior/WeaponBehaviorParser.h"


#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>


namespace weapon_behavior {
namespace {

constexpr char kBinaryMagic[] = "WFSMBIN1";
constexpr std::size_t kBinaryMagicSize = 8;
constexpr std::uint16_t kBinaryVersion = 1;

void logError(IDebug* debug, const std::string& message) {
    if (debug != nullptr) {
        debug->error("WeaponBehaviorParser: " + message);
    }
}

class BinaryReader {
public:
    explicit BinaryReader(const std::string& data) : m_data(data) {}

    [[nodiscard]] bool ok() const { return m_ok; }
    [[nodiscard]] std::size_t remaining() const { return m_data.size() - m_pos; }

    bool expectMagic() {
        if (remaining() < kBinaryMagicSize ||
            std::memcmp(m_data.data() + m_pos, kBinaryMagic, kBinaryMagicSize) != 0) {
            m_ok = false;
            return false;
        }
        m_pos += kBinaryMagicSize;
        return true;
    }

    std::uint8_t readU8() {
        if (!require(1)) return 0;
        return static_cast<std::uint8_t>(m_data[m_pos++]);
    }

    std::uint16_t readU16() {
        if (!require(2)) return 0;
        const auto a = static_cast<std::uint8_t>(m_data[m_pos]);
        const auto b = static_cast<std::uint8_t>(m_data[m_pos + 1]);
        m_pos += 2;
        return static_cast<std::uint16_t>(a | (static_cast<std::uint16_t>(b) << 8));
    }

    std::uint64_t readU64() {
        if (!require(8)) return 0;
        std::uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i) {
            value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(m_data[m_pos + i])) << (i * 8);
        }
        m_pos += 8;
        return value;
    }

    std::int64_t readI64() {
        return static_cast<std::int64_t>(readU64());
    }

    double readDouble() {
        const std::uint64_t bits = readU64();
        double value = 0.0;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    std::string readString() {
        const std::uint16_t length = readU16();
        if (!m_ok || !require(length)) return {};
        std::string value(m_data.data() + m_pos, length);
        m_pos += length;
        return value;
    }

private:
    bool require(const std::size_t count) {
        if (!m_ok || count > remaining()) {
            m_ok = false;
            return false;
        }
        return true;
    }

    const std::string& m_data;
    std::size_t m_pos = 0;
    bool m_ok = true;
};

RuntimeValue readRuntimeValue(BinaryReader& reader) {
    switch (reader.readU8()) {
        case 0: return std::monostate{};
        case 1: return reader.readU8() != 0;
        case 2: return reader.readI64();
        case 3: return reader.readDouble();
        case 4: return reader.readString();
        default: return std::monostate{};
    }
}

VariableComparisonDef readComparison(BinaryReader& reader) {
    VariableComparisonDef result;
    result.name = reader.readString();
    const std::uint8_t mode = reader.readU8();
    if (mode == 1) {
        result.value = readRuntimeValue(reader);
    } else if (mode == 2) {
        result.valueFromVar = reader.readString();
    }
    return result;
}

GuardDef readGuard(BinaryReader& reader) {
    GuardDef guard;
    const std::uint8_t flags = reader.readU8();

    const std::uint16_t allCount = reader.readU16();
    guard.all.reserve(allCount);
    for (std::uint16_t i = 0; i < allCount; ++i) guard.all.push_back(readGuard(reader));

    const std::uint16_t anyCount = reader.readU16();
    guard.any.reserve(anyCount);
    for (std::uint16_t i = 0; i < anyCount; ++i) guard.any.push_back(readGuard(reader));

    if ((flags & (1u << 0)) != 0) guard.triggerPressed = reader.readU8() != 0;
    if ((flags & (1u << 1)) != 0) guard.varEq = readComparison(reader);
    if ((flags & (1u << 2)) != 0) guard.varGt = readComparison(reader);
    if ((flags & (1u << 3)) != 0) guard.varGte = readComparison(reader);
    if ((flags & (1u << 4)) != 0) guard.varLt = readComparison(reader);
    if ((flags & (1u << 5)) != 0) guard.varLte = readComparison(reader);
    return guard;
}

ActionDef readAction(BinaryReader& reader) {
    ActionDef action;
    action.type = reader.readString();
    const std::uint16_t flags = reader.readU16();

    if ((flags & (1u << 0)) != 0) action.clip = reader.readString();
    if ((flags & (1u << 1)) != 0) {
        const std::uint16_t count = reader.readU16();
        action.clips.reserve(count);
        for (std::uint16_t i = 0; i < count; ++i) action.clips.push_back(reader.readString());
    }
    if ((flags & (1u << 2)) != 0) action.mode = reader.readString();
    if ((flags & (1u << 3)) != 0) action.interrupt = reader.readString();
    if ((flags & (1u << 4)) != 0) action.effect = reader.readString();
    if ((flags & (1u << 5)) != 0) action.clipSet = reader.readString();
    if ((flags & (1u << 6)) != 0) action.sequence = reader.readString();
    if ((flags & (1u << 7)) != 0) action.name = reader.readString();
    if ((flags & (1u << 8)) != 0) action.value = readRuntimeValue(reader);
    if ((flags & (1u << 9)) != 0) action.valueFromVar = reader.readString();
    if ((flags & (1u << 10)) != 0) action.delta = static_cast<int>(reader.readI64());
    if ((flags & (1u << 11)) != 0) action.event = reader.readString();
    if ((flags & (1u << 12)) != 0) action.delayMs = static_cast<int>(reader.readI64());
    if ((flags & (1u << 13)) != 0) action.delayMsFromVar = reader.readString();
    if ((flags & (1u << 14)) != 0) action.chance = reader.readDouble();
    if ((flags & (1u << 15)) != 0) action.message = reader.readString();
    return action;
}

std::vector<ActionDef> readActions(BinaryReader& reader) {
    const std::uint16_t count = reader.readU16();
    std::vector<ActionDef> result;
    result.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) result.push_back(readAction(reader));
    return result;
}

bool readHeader(BinaryReader& reader, std::string& initialState, IDebug* debug) {
    if (!reader.expectMagic()) {
        logError(debug, "invalid binary runtime magic");
        return false;
    }
    const std::uint16_t version = reader.readU16();
    if (!reader.ok() || version != kBinaryVersion) {
        logError(debug, "unsupported binary runtime version");
        return false;
    }
    initialState = reader.readString();
    if (!reader.ok()) {
        logError(debug, "truncated binary runtime header");
        return false;
    }
    return true;
}

bool readAssetTables(
    BinaryReader& reader,
    WeaponBehaviorDef* behavior,
    WeaponAssetReferences* references,
    IDebug* debug)
{
    const std::uint16_t clipCount = reader.readU16();
    for (std::uint16_t i = 0; i < clipCount; ++i) {
        const std::string name = reader.readString();
        const std::string path = reader.readString();
        const bool preload = reader.readU8() != 0;
        if (behavior) (*behavior).clips[name] = ClipDef{path, preload};
        if (references) references->audioPaths.push_back(path);
    }

    const std::uint16_t clipSetCount = reader.readU16();
    for (std::uint16_t i = 0; i < clipSetCount; ++i) {
        const std::string name = reader.readString();
        const std::string mode = reader.readString();
        const std::uint16_t itemCount = reader.readU16();
        if (behavior) {
            ClipSetDef clipSet;
            clipSet.mode = mode;
            clipSet.clips.reserve(itemCount);
            for (std::uint16_t j = 0; j < itemCount; ++j) clipSet.clips.push_back(reader.readString());
            (*behavior).clipSets[name] = std::move(clipSet);
        } else {
            for (std::uint16_t j = 0; j < itemCount; ++j) (void)reader.readString();
        }
    }

    const std::uint16_t effectCount = reader.readU16();
    for (std::uint16_t i = 0; i < effectCount; ++i) {
        const std::string name = reader.readString();
        const std::string mode = reader.readString();
        const std::string interrupt = reader.readString();
        const bool loop = reader.readU8() != 0;
        const double gain = reader.readDouble();
        const std::uint16_t itemCount = reader.readU16();
        if (behavior) {
            AudioEffectDef effect;
            effect.mode = mode;
            effect.interrupt = interrupt;
            effect.loop = loop;
            effect.gain = gain;
            effect.clips.reserve(itemCount);
            for (std::uint16_t j = 0; j < itemCount; ++j) effect.clips.push_back(reader.readString());
            (*behavior).audioEffects[name] = std::move(effect);
        } else {
            for (std::uint16_t j = 0; j < itemCount; ++j) (void)reader.readString();
        }
    }

    const std::uint16_t lightCount = reader.readU16();
    for (std::uint16_t i = 0; i < lightCount; ++i) {
        const std::string name = reader.readString();
        const std::string path = reader.readString();
        const bool preload = reader.readU8() != 0;
        if (behavior) (*behavior).lightSequences[name] = LightSequenceDef{path, preload};
        if (references) references->lightSequencePaths.push_back(path);
    }

    if (!reader.ok()) {
        logError(debug, "truncated binary runtime asset tables");
        return false;
    }
    return true;
}

} // namespace

bool WeaponBehaviorParser::isBinaryFormat(const std::string& data) {
    return data.size() >= kBinaryMagicSize && std::memcmp(data.data(), kBinaryMagic, kBinaryMagicSize) == 0;
}

WeaponBehaviorDef WeaponBehaviorParser::parse(const std::string& data, IDebug* debug) {
    if (isBinaryFormat(data)) return parseFromBinary(data, debug);
    logError(debug, "expected WFSMBIN1 binary runtime (re-export SD assets)");
    return {};
}

WeaponAssetReferences WeaponBehaviorParser::parseAssetReferences(const std::string& data, IDebug* debug) {
    WeaponAssetReferences result;
    if (!isBinaryFormat(data)) {
        logError(debug, "expected WFSMBIN1 binary runtime");
        return result;
    }

    BinaryReader reader(data);
    std::string initialState;
    if (!readHeader(reader, initialState, debug)) return result;
    result.valid = readAssetTables(reader, nullptr, &result, debug);
    return result;
}

WeaponBehaviorDef WeaponBehaviorParser::parseFromBinary(const std::string& data, IDebug* debug) {
    WeaponBehaviorDef def;
    BinaryReader reader(data);
    if (!readHeader(reader, def.initialState, debug)) return def;
    def.formatVersion = kBinaryVersion;
    if (!readAssetTables(reader, &def, nullptr, debug)) return def;

    const std::uint16_t variableCount = reader.readU16();
    for (std::uint16_t i = 0; i < variableCount; ++i) {
        const std::string name = reader.readString();
        def.variables[name] = readRuntimeValue(reader);
    }

    const std::uint16_t stateCount = reader.readU16();
    for (std::uint16_t i = 0; i < stateCount; ++i) {
        StateDef state;
        state.id = reader.readString();
        state.label = reader.readString();
        state.onEntry = readActions(reader);
        state.onExit = readActions(reader);
        if (!state.id.empty()) def.states[state.id] = std::move(state);
    }

    const std::uint16_t transitionCount = reader.readU16();
    def.transitions.reserve(transitionCount);
    for (std::uint16_t i = 0; i < transitionCount; ++i) {
        TransitionDef transition;
        transition.id = reader.readString();
        transition.source = reader.readString();
        transition.trigger = reader.readString();
        transition.target = reader.readString();
        transition.internal = reader.readU8() != 0;
        if (reader.readU8() != 0) transition.guard = readGuard(reader);
        transition.actions = readActions(reader);
        def.transitions.push_back(std::move(transition));
    }

    if (!reader.ok()) {
        logError(debug, "truncated binary runtime document");
        return WeaponBehaviorDef{};
    }
    return def;
}

} // namespace weapon_behavior
