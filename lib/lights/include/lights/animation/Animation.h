//
// Created by user on 3/27/2026.
//

#ifndef BLASTERFIRMWARE_ANIMATION_H
#define BLASTERFIRMWARE_ANIMATION_H

#pragma once
#include "Animation.h"
#include "core/lights/ILights.h"

namespace Lights::Animations {

    class IAnimation {
    public:
        virtual ~IAnimation() = default;
        virtual void step(std::vector<LED>& leds) = 0;
    };

    class Blink : public IAnimation {
    public:
        Blink(int r=255, int g=0, int b=0);

        void step(std::vector<LED>& leds) override;

    private:
        int r_, g_, b_;
        bool on_ = false;
    };

} // namespace Lights::Animations

#endif //BLASTERFIRMWARE_ANIMATION_H