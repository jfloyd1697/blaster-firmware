#include "core/weapons/SingleShotMode.h"

#include "core/audio/IAudioEngine.h"
#include "core/time/ITime.h"
#include "core/weapons/WeaponProfile.h"

SingleShotMode::SingleShotMode(const ShootContext &ctx)
    : m_ctx(ctx) {
}

void SingleShotMode::onEquipped() {
    m_lastShotMs = 0;
}

void SingleShotMode::update() {
}

void SingleShotMode::onTriggerPressed() {
    const auto now = static_cast<uint32_t>(m_ctx.time->millis());
    if (now - m_lastShotMs < m_ctx.profile->shootMode.cooldownMs) {
        return;
    }

    if (fireOneShot()) {
        m_lastShotMs = now;
    }
}

void SingleShotMode::onTriggerReleased() {
}

bool SingleShotMode::fireOneShot() const {
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

bool SingleShotMode::consumeAmmo() const {
    if (!m_ctx.ammo || *m_ctx.ammo < m_ctx.profile->ammoPerShot) {
        if (m_ctx.audio && !m_ctx.profile->sounds.empty.empty()) {
            m_ctx.audio->playSound(m_ctx.profile->sounds.empty);
        }
        return false;
    }

    *m_ctx.ammo -= m_ctx.profile->ammoPerShot;
    return true;
}
