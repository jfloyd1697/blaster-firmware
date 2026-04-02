#ifndef BLASTER_FIRMWARE_CHARGESHOTMODE_H
#define BLASTER_FIRMWARE_CHARGESHOTMODE_H

#pragma once

#include "core/weapons/IShootMode.h"
#include "core/weapons/ShootContext.h"

class ChargeShotMode : public IShootMode {
public:
    explicit ChargeShotMode(const ShootContext &ctx);

    void onEquipped() override;
    void update() override;
    void onTriggerPressed() override;
    void onTriggerReleased() override;

protected:
    [[nodiscard]] uint32_t nowMs() const;
    void fireChargedShot() const;
    bool consumeAmmo() const;

private:
    ShootContext m_ctx;
    bool m_isCharging = false;
    bool m_chargeReady = false;
    uint32_t m_chargeStartMs = 0;
};


#endif //BLASTER_FIRMWARE_CHARGESHOTMODE_H