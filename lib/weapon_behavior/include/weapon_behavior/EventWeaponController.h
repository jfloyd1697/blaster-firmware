#ifndef BLASTER_FIRMWARE_EVENTWEAPONCONTROLLER_H
#define BLASTER_FIRMWARE_EVENTWEAPONCONTROLLER_H

#include "WeaponBehaviorEventHandler.h"
#include "core/audio/IAudioEngine.h"
#include "core/weapons/IWeaponController.h"


class EventWeaponController : IWeaponController {
public:
    EventWeaponController(IAudioEngine *audioEngine, WeaponBehaviorDef *behaviorDef);

    void equipWeapon(const WeaponProfile &profile) override;

    void reloadWeapon() override;

    void onEmpty() override;

    void onTriggerPressed() override;

    void onTriggerHeld() override;

    void onTriggerReleased() override;

private:
    WeaponBehaviorDef *m_behaviorDef = nullptr;
    IAudioEngine *audio;
    std::unique_ptr<WeaponBehaviorEventHandler> m_behaviorController;

};


#endif //BLASTER_FIRMWARE_EVENTWEAPONCONTROLLER_H