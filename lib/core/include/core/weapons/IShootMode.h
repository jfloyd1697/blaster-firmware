#ifndef BLASTER_FIRMWARE_ISHOOTMODE_H
#define BLASTER_FIRMWARE_ISHOOTMODE_H

#pragma once

class IShootMode {
public:
    virtual ~IShootMode() = default;

    virtual void onEquipped() = 0;
    virtual void update() = 0;
    virtual void onTriggerPressed() = 0;
    virtual void onTriggerReleased() = 0;
};

#endif // BLASTER_FIRMWARE_ISHOOTMODE_H
