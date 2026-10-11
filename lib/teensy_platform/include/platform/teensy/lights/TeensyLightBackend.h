#ifndef BLASTER_TEENSY_LIGHT_BACKEND_H
#define BLASTER_TEENSY_LIGHT_BACKEND_H

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/lights/ILightBackend.h"

class OctoWS2811;

class TeensyLightBackend : public ILightBackend {
public:
    TeensyLightBackend();
    ~TeensyLightBackend() override;

    bool begin() override;
    [[nodiscard]] std::size_t ledCount() const override;
    void show(const std::vector<LightColor>& pixels) override;

private:
    OctoWS2811* m_leds = nullptr;
};

#endif // BLASTER_TEENSY_LIGHT_BACKEND_H
