#ifndef BLASTER_FIRMWARE_ILIGHTBACKEND_H
#define BLASTER_FIRMWARE_ILIGHTBACKEND_H

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using LightColor = std::array<std::uint8_t, 3>;

class ILightBackend {
public:
    virtual ~ILightBackend() = default;

    virtual bool begin() = 0;
    [[nodiscard]] virtual std::size_t ledCount() const = 0;
    virtual void show(const std::vector<LightColor>& pixels) = 0;
};

#endif // BLASTER_FIRMWARE_ILIGHTBACKEND_H
