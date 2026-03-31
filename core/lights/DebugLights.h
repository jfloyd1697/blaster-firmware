//
// Created by user on 3/27/2026.
//

#ifndef BLASTERFIRMWARE_DEBUGLIGHTS_H
#define BLASTERFIRMWARE_DEBUGLIGHTS_H

#pragma once
#include "../../include/ILights.h"
#include <memory>
#include "../../include/IDebug.h"

namespace Lights {

    class DebugLights : public ILights {
    public:
        DebugLights(std::shared_ptr<ILights> wrapped, std::shared_ptr<IDebug> debug)
            : wrapped_(std::move(wrapped)), debug_(std::move(debug))
        {}

        void step() override {;
            if (debug_) debug_->log("Stepping DebugLights");
            wrapped_->step();
        }

        void addAnimation(std::shared_ptr<Animations::IAnimation> anim) override {
            if (debug_) debug_->log("Adding animation to DebugLights");
            wrapped_->addAnimation(anim);
        }
        [[nodiscard]] const std::vector<LED>& getLEDs() const override {;
            if (debug_) debug_->log("Getting LEDs from DebugLights");
            return wrapped_->getLEDs();
        }

        void setDebugService(const std::shared_ptr<IDebug> &debug) { debug_ = debug; }

    private:
        std::shared_ptr<ILights> wrapped_;
        std::shared_ptr<IDebug> debug_;
    };

} // namespace Lights

#endif //BLASTERFIRMWARE_DEBUGLIGHTS_H