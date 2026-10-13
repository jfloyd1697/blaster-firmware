#ifndef BLASTER_FIRMWARE_ISTATUSDISPLAY_H
#define BLASTER_FIRMWARE_ISTATUSDISPLAY_H

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// A snapshot of presentation data. The weapon runtime never performs I2C transfers.
enum class DisplayWeaponState : std::uint8_t {
    Loading,
    Ready,
    Error,
};

enum class DisplayControlMode : std::uint8_t {
    Normal,
    RemoteInput,
    DirectControl,
};

struct DisplayProgress {
    std::size_t completed = 0;
    std::size_t total = 0;
};

struct DisplayStatus {
    std::string bankName;
    std::string weaponName;
    std::string behaviorState;
    std::size_t bankIndex = 0;
    std::size_t bankCount = 0;
    std::size_t weaponIndex = 0;
    std::size_t weaponCount = 0;
    DisplayWeaponState weaponState = DisplayWeaponState::Loading;
    std::optional<DisplayProgress> progress;
};

class IStatusDisplay {
public:
    virtual ~IStatusDisplay() = default;

    // Must be cheap and I/O-free: may be called in the real-time control tick.
    virtual void present(const DisplayStatus& status) = 0;
    virtual void setControlMode(DisplayControlMode mode) = 0;

    // Bounded, best-effort background I/O. Never called by audio/light ISRs.
    virtual void service(std::uint32_t nowMs) = 0;
};

#endif // BLASTER_FIRMWARE_ISTATUSDISPLAY_H
