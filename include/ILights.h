//
// Created by user on 3/27/2026.
//

#ifndef BLASTERFIRMWARE_ILIGHTS_H
#define BLASTERFIRMWARE_ILIGHTS_H

#pragma once
#include <vector>
#include <memory>

namespace Lights {

    namespace Animations {
        class IAnimation;
    }

    struct LED {
        int r = 0, g = 0, b = 0;
    };

    class ILights {
    public:
        virtual ~ILights() = default;

        virtual void update() = 0;
        virtual void addAnimation(std::shared_ptr<Animations::IAnimation> anim) = 0;
        [[nodiscard]] virtual const std::vector<LED>& getLEDs() const = 0;
    };

} // namespace Lights

#endif //BLASTERFIRMWARE_ILIGHTS_H