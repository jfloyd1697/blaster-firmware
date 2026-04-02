//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_RAPIDFIREMODE_H
#define BLASTER_FIRMWARE_RAPIDFIREMODE_H


#pragma once

#include <cstdint>

#include "core/weapons/IShootMode.h"
#include "core/weapons/ShootContext.h"

class RapidFireMode : public IShootMode {
public:
    explicit RapidFireMode(const ShootContext &ctx);

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
    bool m_triggerHeld = false;
    uint32_t m_lastShotMs = 0;
};

#endif //BLASTER_FIRMWARE_RAPIDFIREMODE_H