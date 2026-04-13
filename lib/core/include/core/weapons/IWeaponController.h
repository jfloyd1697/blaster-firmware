//
// Created by user on 4/13/2026.
//

#ifndef BLASTER_FIRMWARE_WEAPONCONTROLLER_H
#define BLASTER_FIRMWARE_WEAPONCONTROLLER_H
#include "WeaponProfile.h"


class IWeaponController {
public:
    virtual ~IWeaponController() = default;

    virtual void equipWeapon(const WeaponProfile &profile);

    virtual void reloadWeapon();

    virtual void onEmpty();

    virtual void onTriggerPressed();

    virtual void onTriggerHeld();

    virtual void onTriggerReleased();

    virtual void update();
};


#endif //BLASTER_FIRMWARE_WEAPONCONTROLLER_H
