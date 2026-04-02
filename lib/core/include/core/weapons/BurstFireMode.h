//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_BURSTFIREMODE_H
#define BLASTER_FIRMWARE_BURSTFIREMODE_H

#pragma once

#include <cstdint>

#include "core/weapons/IShootMode.h"
#include "core/weapons/ShootContext.h"

class BurstFireMode : public IShootMode {
public:
    explicit BurstFireMode(const ShootContext &ctx);

    void onEquipped() override;
    void update() override;
    void onTriggerPressed() override;
    void onTriggerReleased() override;

protected:
    [[nodiscard]] uint32_t nowMs() const;
    bool fireOneShot() const;
    bool consumeAmmo() const;

private:
    ShootContext m_ctx;
    bool m_burstActive = false;
    int m_shotsRemaining = 0;
    uint32_t m_nextShotMs = 0;
};

#endif //BLASTER_FIRMWARE_BURSTFIREMODE_H