//
// Created by user on 3/30/2026.
//

#ifndef BLASTERFIRMWARE_BUTTONTRACKER_H
#define BLASTERFIRMWARE_BUTTONTRACKER_H

#pragma once

#include <cstdint>

class ButtonTracker {
public:
    void update(bool rawPressed, std::uint32_t nowMs);

    bool isPressed() const;

    bool wasPressed() const;
    bool wasReleased() const;
    bool wasShortPressed() const;
    bool wasLongPressed() const;

private:
    bool pressed_ = false;
    bool lastRawPressed_ = false;

    bool pressedEvent_ = false;
    bool releasedEvent_ = false;
    bool shortPressedEvent_ = false;
    bool longPressedEvent_ = false;

    bool longReported_ = false;

    std::uint32_t lastChangeMs_ = 0;
    std::uint32_t pressStartMs_ = 0;

    static constexpr std::uint32_t debounceMs_ = 20;
    static constexpr std::uint32_t longPressMs_ = 500;
};

#endif //BLASTERFIRMWARE_BUTTONTRACKER_H