#ifndef BLASTER_FIRMWARE_SSD1306_STATUS_DISPLAY_H
#define BLASTER_FIRMWARE_SSD1306_STATUS_DISPLAY_H

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <Wire.h>

#include "core/display/IStatusDisplay.h"

// Small 128x32 SSD1306 I2C module. No graphics-library allocation or bulk I2C flush.
// A single service() call writes at most one page command or a short data chunk.
class Ssd1306StatusDisplay final : public IStatusDisplay {
public:
    explicit Ssd1306StatusDisplay(TwoWire& bus, std::uint8_t address = 0x3C);

    bool begin();
    void present(const DisplayStatus& status) override;
    void setControlMode(DisplayControlMode mode) override;
    void service(std::uint32_t nowMs) override;

private:
    static constexpr std::size_t kWidth = 128;
    static constexpr std::size_t kPages = 4;
    static constexpr std::size_t kGlyphWidth = 6;
    static constexpr std::size_t kVisibleChars = kWidth / kGlyphWidth;
    static constexpr std::size_t kDataBytesPerTransaction = 8;
    static constexpr std::uint32_t kScrollIntervalMs = 450;

    bool sendCommand(const std::uint8_t* values, std::size_t count);
    void render();
    void drawLine(std::size_t page, const std::string& text);
    void drawProgress(std::size_t completed, std::size_t total);
    [[nodiscard]] std::string weaponLine() const;

    TwoWire& m_bus;
    std::uint8_t m_address;
    std::array<std::uint8_t, kWidth * kPages> m_frame{};
    DisplayStatus m_status;
    DisplayControlMode m_mode = DisplayControlMode::Normal;
    std::size_t m_scrollOffset = 0;
    std::size_t m_page = 0;
    std::size_t m_column = 0;
    std::uint32_t m_lastScrollMs = 0;
    bool m_enabled = false;
    bool m_dirty = true;
    bool m_needsPageCommand = true;
    bool m_scrollTimerNeedsReset = true;
};

#endif // BLASTER_FIRMWARE_SSD1306_STATUS_DISPLAY_H
