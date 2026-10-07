#ifndef BLASTER_FIRMWARE_ILIGHTS_H
#define BLASTER_FIRMWARE_ILIGHTS_H

#pragma once

namespace weapon_behavior {
struct LightPatternDef;
}

class ILights {
public:
    virtual ~ILights() = default;

    virtual void setPattern(const weapon_behavior::LightPatternDef& pattern) = 0;
    virtual void flashPattern(const weapon_behavior::LightPatternDef& pattern) = 0;
    virtual void flash() = 0;
    virtual void update() = 0;
};

#endif // BLASTER_FIRMWARE_ILIGHTS_H
