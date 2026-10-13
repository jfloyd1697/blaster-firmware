#include "display/Ssd1306StatusDisplay.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace {
constexpr std::uint8_t kCommandControlByte = 0x00;
constexpr std::uint8_t kDataControlByte = 0x40;
constexpr std::uint8_t kPageAddressBase = 0xB0;
constexpr std::size_t kGlyphColumns = 5;

std::array<std::uint8_t, kGlyphColumns> glyphFor(char value) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(value)))) {
    case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00};
    case 'A': return {0x7E, 0x09, 0x09, 0x09, 0x7E};
    case 'B': return {0x7F, 0x49, 0x49, 0x49, 0x36};
    case 'C': return {0x3E, 0x41, 0x41, 0x41, 0x41};
    case 'D': return {0x7F, 0x41, 0x41, 0x41, 0x3E};
    case 'E': return {0x7F, 0x49, 0x49, 0x49, 0x41};
    case 'F': return {0x7F, 0x09, 0x09, 0x09, 0x01};
    case 'G': return {0x3E, 0x41, 0x49, 0x49, 0x79};
    case 'H': return {0x7F, 0x08, 0x08, 0x08, 0x7F};
    case 'I': return {0x41, 0x41, 0x7F, 0x41, 0x41};
    case 'J': return {0x30, 0x40, 0x41, 0x3F, 0x01};
    case 'K': return {0x7F, 0x08, 0x14, 0x22, 0x41};
    case 'L': return {0x7F, 0x40, 0x40, 0x40, 0x40};
    case 'M': return {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    case 'N': return {0x7F, 0x02, 0x04, 0x08, 0x7F};
    case 'O': return {0x3E, 0x41, 0x41, 0x41, 0x3E};
    case 'P': return {0x7F, 0x09, 0x09, 0x09, 0x06};
    case 'Q': return {0x3E, 0x41, 0x51, 0x21, 0x5E};
    case 'R': return {0x7F, 0x09, 0x19, 0x29, 0x46};
    case 'S': return {0x46, 0x49, 0x49, 0x49, 0x31};
    case 'T': return {0x01, 0x01, 0x7F, 0x01, 0x01};
    case 'U': return {0x3F, 0x40, 0x40, 0x40, 0x3F};
    case 'V': return {0x1F, 0x20, 0x40, 0x20, 0x1F};
    case 'W': return {0x7F, 0x20, 0x18, 0x20, 0x7F};
    case 'X': return {0x63, 0x14, 0x08, 0x14, 0x63};
    case 'Y': return {0x03, 0x04, 0x78, 0x04, 0x03};
    case 'Z': return {0x61, 0x51, 0x49, 0x45, 0x43};
    case '0': return {0x3E, 0x51, 0x49, 0x45, 0x3E};
    case '1': return {0x00, 0x42, 0x7F, 0x40, 0x00};
    case '2': return {0x42, 0x61, 0x51, 0x49, 0x46};
    case '3': return {0x41, 0x49, 0x49, 0x49, 0x36};
    case '4': return {0x18, 0x14, 0x12, 0x7F, 0x10};
    case '5': return {0x27, 0x45, 0x45, 0x45, 0x39};
    case '6': return {0x3E, 0x49, 0x49, 0x49, 0x30};
    case '7': return {0x01, 0x71, 0x09, 0x05, 0x03};
    case '8': return {0x36, 0x49, 0x49, 0x49, 0x36};
    case '9': return {0x06, 0x49, 0x49, 0x49, 0x3E};
    case '-': return {0x08, 0x08, 0x08, 0x08, 0x08};
    case '_': return {0x40, 0x40, 0x40, 0x40, 0x40};
    case '/': return {0x60, 0x10, 0x08, 0x04, 0x03};
    case '.': return {0x00, 0x60, 0x60, 0x00, 0x00};
    case ':': return {0x00, 0x36, 0x36, 0x00, 0x00};
    case '!': return {0x00, 0x00, 0x5F, 0x00, 0x00};
    case '?': return {0x02, 0x01, 0x51, 0x09, 0x06};
    case '+': return {0x08, 0x08, 0x3E, 0x08, 0x08};
    case '%': return {0x63, 0x13, 0x28, 0x66, 0x41};
    case '#': return {0x14, 0x7F, 0x14, 0x7F, 0x14};
    case '(': return {0x1C, 0x22, 0x41, 0x41, 0x00};
    case ')': return {0x00, 0x41, 0x41, 0x22, 0x1C};
    case '&': return {0x36, 0x49, 0x5D, 0x22, 0x50};
    case '\'': return {0x00, 0x04, 0x03, 0x00, 0x00};
    case ',': return {0x00, 0x00, 0x70, 0x30, 0x00};
    case '=': return {0x14, 0x14, 0x14, 0x14, 0x14};
    default: return {0x00, 0x00, 0x00, 0x00, 0x00};
    }
}

std::string counter(const std::size_t current, const std::size_t total) {
    return std::to_string(current) + "/" + std::to_string(total);
}
} // namespace

Ssd1306StatusDisplay::Ssd1306StatusDisplay(TwoWire& bus, const std::uint8_t address)
    : m_bus(bus), m_address(address) {
}

bool Ssd1306StatusDisplay::sendCommand(const std::uint8_t* values, const std::size_t count) {
    m_bus.beginTransmission(m_address);
    m_bus.write(kCommandControlByte);
    for (std::size_t index = 0; index < count; ++index) {
        m_bus.write(values[index]);
    }
    return m_bus.endTransmission() == 0;
}

bool Ssd1306StatusDisplay::begin() {
    // The display is optional. A failed initial probe disables future transactions.
    m_bus.beginTransmission(m_address);
    if (m_bus.endTransmission() != 0) return false;

    // SSD1306, 128x32 pixels, internal charge pump, page addressing mode.
    static constexpr std::uint8_t kInit[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x1F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x02,
        0x81, 0x8F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0x2E, 0xAF,
    };
    for (const std::uint8_t value : kInit) {
        if (!sendCommand(&value, 1)) return false;
    }
    m_enabled = true;
    m_dirty = true;
    return true;
}

void Ssd1306StatusDisplay::present(const DisplayStatus& status) {
    if (m_status.weaponName != status.weaponName || m_status.bankName != status.bankName) {
        m_scrollOffset = 0;
        m_scrollTimerNeedsReset = true;
    }
    m_status = status;
    m_dirty = true;
}

void Ssd1306StatusDisplay::setControlMode(const DisplayControlMode mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    m_dirty = true;
}

std::string Ssd1306StatusDisplay::weaponLine() const {
    if (m_status.weaponName.size() <= kVisibleChars) return m_status.weaponName;
    // Wrap with a gap so the whole name can be read on a 21-column display.
    const std::string scrolling = m_status.weaponName + "   ";
    std::string visible;
    visible.reserve(kVisibleChars);
    for (std::size_t i = 0; i < kVisibleChars; ++i) {
        visible += scrolling[(m_scrollOffset + i) % scrolling.size()];
    }
    return visible;
}

void Ssd1306StatusDisplay::drawLine(const std::size_t page, const std::string& text) {
    if (page >= kPages) return;
    const std::size_t count = std::min(kVisibleChars, text.size());
    for (std::size_t index = 0; index < count; ++index) {
        const auto glyph = glyphFor(text[index]);
        const std::size_t x = index * kGlyphWidth;
        for (std::size_t column = 0; column < glyph.size(); ++column) {
            m_frame[page * kWidth + x + column] = glyph[column];
        }
    }
}

void Ssd1306StatusDisplay::render() {
    m_frame.fill(0);
    drawLine(0, "B" + counter(m_status.bankIndex + 1, m_status.bankCount) + " " + m_status.bankName);
    drawLine(1, m_status.weaponName.empty() ? "NO WEAPON" : weaponLine());
    std::string state;
    switch (m_status.weaponState) {
    case DisplayWeaponState::Loading: state = "LOADING"; break;
    case DisplayWeaponState::Ready: state = "READY"; break;
    case DisplayWeaponState::Error: state = "ERROR"; break;
    }
    if (m_mode == DisplayControlMode::RemoteInput) state = "PC INPUT";
    if (m_mode == DisplayControlMode::DirectControl) state = "PC DIRECT";
    drawLine(2, "W" + counter(m_status.weaponIndex + 1, m_status.weaponCount) + " " + state);
    drawLine(3, "FSM " + m_status.behaviorState);

}

void Ssd1306StatusDisplay::service(const std::uint32_t nowMs) {
    if (!m_enabled) return;
    if (m_scrollTimerNeedsReset) {
        m_lastScrollMs = nowMs;
        m_scrollTimerNeedsReset = false;
    }
    if (m_status.weaponName.size() > kVisibleChars &&
        nowMs - m_lastScrollMs >= kScrollIntervalMs) {
        const std::size_t span = m_status.weaponName.size() + 3;
        m_scrollOffset = (m_scrollOffset + 1) % span;
        m_lastScrollMs = nowMs;
        m_dirty = true;
    }
    if (m_dirty) {
        render();
        m_dirty = false;
        m_page = 0;
        m_column = 0;
        m_needsPageCommand = true;
    }
    if (m_page >= kPages) return;

    if (m_needsPageCommand) {
        const std::uint8_t commands[] = {
            static_cast<std::uint8_t>(kPageAddressBase | m_page), 0x00, 0x10,
        };
        m_enabled = sendCommand(commands, sizeof(commands));
        m_needsPageCommand = false;
        return;
    }
    m_bus.beginTransmission(m_address);
    m_bus.write(kDataControlByte);
    const std::size_t count = std::min(kDataBytesPerTransaction, kWidth - m_column);
    m_bus.write(m_frame.data() + m_page * kWidth + m_column, count);
    if (m_bus.endTransmission() != 0) {
        m_enabled = false;
        return;
    }
    m_column += count;
    if (m_column == kWidth) {
        m_column = 0;
        ++m_page;
        m_needsPageCommand = true;
    }
}
