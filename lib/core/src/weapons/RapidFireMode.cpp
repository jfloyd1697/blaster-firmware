#include "core/weapons/RapidFireMode.h"

#include "core/audio/IAudioEngine.h"
#include "core/time/ITime.h"
#include "core/weapons/WeaponProfile.h"

RapidFireMode::RapidFireMode(const ShootContext &ctx)
    : m_ctx(ctx) {
}

void RapidFireMode::onEquipped() {
    m_triggerHeld = false;
    m_lastShotMs = 0;
}

void RapidFireMode::update() {
    if (!m_triggerHeld) {
        return;
    }

    const uint32_t now = nowMs();
    if (now - m_lastShotMs >= m_ctx.profile->shootMode.cooldownMs) {
        if (fireOneShot()) {
            m_lastShotMs = now;
        }
    }
}

void RapidFireMode::onTriggerPressed() {
    m_triggerHeld = true;

    const uint32_t now = nowMs();
    if (now - m_lastShotMs >= m_ctx.profile->shootMode.cooldownMs) {
        if (fireOneShot()) {
            m_lastShotMs = now;
        }
    }
}

void RapidFireMode::onTriggerReleased() {
    m_triggerHeld = false;
}

uint32_t RapidFireMode::nowMs() const {
    return static_cast<uint32_t>(m_ctx.time->millis());
}

bool RapidFireMode::fireOneShot() const {
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

bool RapidFireMode::consumeAmmo() const {
    if (!m_ctx.ammo || *m_ctx.ammo < m_ctx.profile->ammoPerShot) {
        if (m_ctx.audio && !m_ctx.profile->sounds.empty.empty()) {
            m_ctx.audio->playSound(m_ctx.profile->sounds.empty);
        }
        return false;
    }

    *m_ctx.ammo -= m_ctx.profile->ammoPerShot;
    return true;
}
