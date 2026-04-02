#include <cstdint>

#include "core/weapons/ChargeShotMode.h"

#include "core/audio/IAudioEngine.h"
#include "core/time/ITime.h"
#include "core/weapons/WeaponProfile.h"

ChargeShotMode::ChargeShotMode(const ShootContext &ctx)
    : m_ctx(ctx) {
}

void ChargeShotMode::onEquipped() {
    m_isCharging = false;
    m_chargeReady = false;
    m_chargeStartMs = 0;
}

void ChargeShotMode::update() {
    if (!m_isCharging) {
        return;
    }

    const uint32_t heldMs = nowMs() - m_chargeStartMs;
    if (heldMs >= m_ctx.profile->shootMode.maxChargeMs) {
        m_chargeReady = true;
    }
}

void ChargeShotMode::onTriggerPressed() {
    if (m_isCharging) {
        return;
    }

    m_isCharging = true;
    m_chargeReady = false;
    m_chargeStartMs = nowMs();

    if (m_ctx.audio && !m_ctx.profile->sounds.chargeStart.empty()) {
        m_ctx.audio->playSound(m_ctx.profile->sounds.chargeStart);
    }
}

void ChargeShotMode::onTriggerReleased() {
    if (!m_isCharging) {
        return;
    }

    const uint32_t heldMs = nowMs() - m_chargeStartMs;
    m_isCharging = false;

    if (heldMs < m_ctx.profile->shootMode.minChargeMs) {
        return;
    }

    fireChargedShot();
}

uint32_t ChargeShotMode::nowMs() const {
    return static_cast<uint32_t>(m_ctx.time->millis());
}

void ChargeShotMode::fireChargedShot() const {
    if (!consumeAmmo()) {
        return;
    }

    const std::string& sound =
        !m_ctx.profile->sounds.chargeRelease.empty()
            ? m_ctx.profile->sounds.chargeRelease
            : m_ctx.profile->sounds.shot;

    if (m_ctx.audio && !sound.empty()) {
        m_ctx.audio->playSound(sound);
    }

    if (m_ctx.emitShot) {
        m_ctx.emitShot();
    }

    if (m_ctx.flashMuzzle) {
        m_ctx.flashMuzzle();
    }
}

bool ChargeShotMode::consumeAmmo() const {
    if (!m_ctx.ammo || *m_ctx.ammo < m_ctx.profile->ammoPerShot) {
        if (m_ctx.audio && !m_ctx.profile->sounds.empty.empty()) {
            m_ctx.audio->playSound(m_ctx.profile->sounds.empty);
        }
        return false;
    }

    *m_ctx.ammo -= m_ctx.profile->ammoPerShot;
    return true;
}
