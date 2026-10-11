#include "core/weapons/WeaponsManifestParser.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/debug/IDebug.h"

namespace {
constexpr char kMagic[] = "WFMAN001";
constexpr std::size_t kMaxEntries = 1024;

class Reader {
public:
    explicit Reader(const std::string& bytes) : m_bytes(bytes) {}

    bool magic() {
        if (m_bytes.size() < 10 || m_bytes.compare(0, 8, kMagic, 8) != 0) return false;
        m_position = 8;
        std::uint16_t version = 0;
        return u16(version) && version == 1;
    }

    bool u16(std::uint16_t& result) {
        if (m_position > m_bytes.size() || m_bytes.size() - m_position < 2) return false;
        const auto lo = static_cast<std::uint8_t>(m_bytes[m_position]);
        const auto hi = static_cast<std::uint8_t>(m_bytes[m_position + 1]);
        result = static_cast<std::uint16_t>(lo | (static_cast<unsigned>(hi) << 8));
        m_position += 2;
        return true;
    }

    bool string(std::string& result) {
        std::uint16_t count = 0;
        if (!u16(count) || count > m_bytes.size() - m_position) return false;
        result.assign(m_bytes.data() + m_position, count);
        m_position += count;
        return true;
    }

    bool finished() const { return m_position == m_bytes.size(); }

private:
    const std::string& m_bytes;
    std::size_t m_position = 0;
};

void error(IDebug* debug, const char* message) {
    if (debug) debug->error(std::string("WeaponsManifestParser: ") + message);
}

bool safeWeaponPath(const std::string& path) {
    if (path.size() < 4 || path.substr(path.size() - 4) != ".bin" ||
        path.front() == '/' || path.find('\\') != std::string::npos) return false;
    std::size_t begin = 0;
    while (begin < path.size()) {
        const std::size_t end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string::npos ? end : end - begin);
        if (segment == ".." || segment == "." || segment.empty()) return false;
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return true;
}
} // namespace

WeaponsManifest WeaponsManifestParser::parseFromText(const std::string& bytes, IDebug* debug) {
    WeaponsManifest manifest;
    Reader input(bytes);
    if (!input.magic()) {
        error(debug, "expected WFMAN001 binary manifest, version 1 (re-export SD assets)");
        return manifest;
    }
    std::uint16_t bankCount = 0;
    if (!input.u16(bankCount) || bankCount == 0 || bankCount > kMaxEntries) {
        error(debug, "invalid bank count");
        return {};
    }
    for (std::uint16_t i = 0; i < bankCount; ++i) {
        WeaponBankManifest bank;
        std::uint16_t weaponCount = 0;
        if (!input.string(bank.name) || bank.name.empty() ||
            !input.u16(weaponCount) || weaponCount == 0 || weaponCount > kMaxEntries) {
            error(debug, "invalid bank entry");
            return {};
        }
        for (std::uint16_t j = 0; j < weaponCount; ++j) {
            std::string weaponPath;
            if (!input.string(weaponPath) || !safeWeaponPath(weaponPath)) {
                error(debug, "invalid weapon path (expected relative .bin path)");
                return {};
            }
            bank.weapons.push_back(std::move(weaponPath));
        }
        manifest.banks.push_back(std::move(bank));
    }
    if (!input.finished()) {
        error(debug, "trailing manifest data");
        return {};
    }
    manifest.version = 1;
    return manifest;
}
