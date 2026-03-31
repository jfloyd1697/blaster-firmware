#include "ButtonTracker.h"

void ButtonTracker::update(const bool rawPressed, const std::uint32_t nowMs) {
    pressedEvent_ = false;
    releasedEvent_ = false;
    shortPressedEvent_ = false;
    longPressedEvent_ = false;

    if (rawPressed != lastRawPressed_) {
        lastRawPressed_ = rawPressed;
        lastChangeMs_ = nowMs;
    }

    if ((nowMs - lastChangeMs_) >= debounceMs_) {
        if (pressed_ != rawPressed) {
            pressed_ = rawPressed;

            if (pressed_) {
                pressedEvent_ = true;
                pressStartMs_ = nowMs;
                longReported_ = false;
            } else {
                releasedEvent_ = true;

                if (!longReported_) {
                    shortPressedEvent_ = true;
                }
            }
        }
    }

    if (pressed_ && !longReported_ && (nowMs - pressStartMs_ >= longPressMs_)) {
        longReported_ = true;
        longPressedEvent_ = true;
    }
}

bool ButtonTracker::isPressed() const {
    return pressed_;
}

bool ButtonTracker::wasPressed() const {
    return pressedEvent_;
}

bool ButtonTracker::wasReleased() const {
    return releasedEvent_;
}

bool ButtonTracker::wasShortPressed() const {
    return shortPressedEvent_;
}

bool ButtonTracker::wasLongPressed() const {
    return longPressedEvent_;
}