//
// Created by user on 3/27/2026.
//

#ifndef BLASTERFIRMWARE_LIGHTENGINE_H
#define BLASTERFIRMWARE_LIGHTENGINE_H

#pragma once
#include "../../include/ILights.h"
#include <vector>
#include <memory>

#include "Animation.h"

namespace Lights {

    class LedLights : public ILights {
    public:
        LedLights(size_t numLEDs);

        void step() override;
        void addAnimation(std::shared_ptr<Animations::IAnimation> anim) override;
        [[nodiscard]] const std::vector<LED>& getLEDs() const override;

    private:
        std::vector<LED> leds_;
        std::vector<std::shared_ptr<Animations::IAnimation>> animations_;
    };

    std::shared_ptr<ILights> createLights(size_t numLEDs);

    class LightEngine {
    public:
        LightEngine(ILights *lights) : m_lights(lights) {};

        void blink(int r, int g, int b) {
            // Example: Add a simple blink animation
            m_lights->addAnimation(std::make_shared<Animations::Blink>(r, g, b));
        }
        void step() {
            m_lights->step();
        }

    private:
        ILights *m_lights;
    };

} // namespace Lights

#endif //BLASTERFIRMWARE_LIGHTENGINE_H