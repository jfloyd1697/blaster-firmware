//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_SINGLESHOTMODE_H
#define BLASTER_FIRMWARE_SINGLESHOTMODE_H


#pragma once

#include <cstdint>

#include "core/weapons/IShootMode.h"
#include "core/weapons/ShootContext.h"

class SingleShotMode : public IShootMode {
public:
    explicit SingleShotMode(const ShootContext &ctx);

    void onEquipped() override;
    void update() override;
    void onTriggerPressed() override;
    void onTriggerReleased() override;

protected:
    [[nodiscard]] bool fireOneShot() const;
    [[nodiscard]] bool consumeAmmo() const;

private:
    ShootContext m_ctx;
    uint32_t m_lastShotMs = 0;
};

#endif //BLASTER_FIRMWARE_SINGLESHOTMODE_H