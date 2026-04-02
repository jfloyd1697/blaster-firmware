#include "core/weapons/BurstFireMode.h"

#include "core/audio/IAudioEngine.h"
#include "core/time/ITime.h"
#include "core/weapons/WeaponProfile.h"

BurstFireMode::BurstFireMode(const ShootContext &ctx)
    : m_ctx(ctx) {
}

void BurstFireMode::onEquipped() {
    m_burstActive = false;
    m_shotsRemaining = 0;
    m_nextShotMs = 0;
}

void BurstFireMode::update() {
    if (!m_burstActive) {
        return;
    }

    const uint32_t now = nowMs();
    if (now < m_nextShotMs) {
        return;
    }

    if (!fireOneShot()) {
        m_burstActive = false;
        m_shotsRemaining = 0;
        return;
    }

    --m_shotsRemaining;
    if (m_shotsRemaining <= 0) {
        m_burstActive = false;
    } else {
        m_nextShotMs = now + m_ctx.profile->shootMode.burstIntervalMs;
    }
}

void BurstFireMode::onTriggerPressed() {
    if (m_burstActive) {
        return;
    }

    m_burstActive = true;
    m_shotsRemaining = m_ctx.profile->shootMode.burstCount;
    m_nextShotMs = nowMs();
}

void BurstFireMode::onTriggerReleased() {
}

uint32_t BurstFireMode::nowMs() const {
    return static_cast<uint32_t>(m_ctx.time->millis());
}

bool BurstFireMode::fireOneShot() const {
    if (!consumeAmmo()) {
        return false;
    }

    if (m_ctx.audio && !m_ctx.profile->sounds.shot.empty()) {
        m_ctx.audio->playSound(m_ctx.profile->sounds.shot);
    }

    if (m_ctx.emitShot) {
        m_ctx.emitShot();
    }

    if (m_ctx.flashMuzzle) {
        m_ctx.flashMuzzle();
    }

    return true;
}

bool BurstFireMode::consumeAmmo() const {
    if (!m_ctx.ammo || *m_ctx.ammo < m_ctx.profile->ammoPerShot) {
        if (m_ctx.audio && !m_ctx.profile->sounds.empty.empty()) {
            m_ctx.audio->playSound(m_ctx.profile->sounds.empty);
        }
        return false;
    }

    *m_ctx.ammo -= m_ctx.profile->ammoPerShot;
    return true;
}
