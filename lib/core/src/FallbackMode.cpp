#include "core/FallbackMode.h"

#include "core/debug/IDebug.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace {
weapon_behavior::LightPatternDef fallbackPattern() {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Pulse;
    pattern.color = {255, 80, 0};
    pattern.brightness = 96;
    pattern.intervalMs = 1200;
    return pattern;
}

weapon_behavior::LightPatternDef triggerFlashPattern() {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Solid;
    pattern.color = {255, 255, 255};
    pattern.brightness = 255;
    pattern.durationMs = 80;
    return pattern;
}
}

FallbackMode::FallbackMode(PlatformServices& services)
    : m_services(services) {
}

void FallbackMode::initialize() {
    if (m_services.debug) {
        m_services.debug->error("Entering fallback mode: weapon manifest unavailable");
        m_services.debug->log("Fallback mode: trigger flashes the lights; storage can be diagnosed with the platform test build");
    }

    if (m_services.lights) {
        m_services.lights->setPattern(fallbackPattern());
    }
}

bool FallbackMode::update() {
    if (!m_services.input) {
        return true;
    }

    if (m_services.input->wasTriggerPressed()) {
        if (m_services.debug) {
            m_services.debug->log("Fallback mode: trigger pressed");
        }
        if (m_services.lights) {
            m_services.lights->flashPattern(triggerFlashPattern());
        }
    }

    return !m_services.input->wasQuitPressed();
}
