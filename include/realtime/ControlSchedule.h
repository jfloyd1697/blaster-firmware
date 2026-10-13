#ifndef BLASTER_REALTIME_CONTROL_SCHEDULE_H
#define BLASTER_REALTIME_CONTROL_SCHEDULE_H

#include <cstdint>

namespace realtime {

// Wrap-safe, no catch-up bursts: a late control tick runs once, then the
// following tick is scheduled relative to its original periodic phase.
class ControlSchedule {
public:
    [[nodiscard]] bool due(std::uint32_t nowUs, std::uint32_t periodUs) {
        if (!initialized_) {
            deadlineUs_ = nowUs;
            initialized_ = true;
        }
        if (static_cast<std::int32_t>(nowUs - deadlineUs_) < 0) return false;

        const std::uint32_t lateness = nowUs - deadlineUs_;
        if (lateness > maximumLatenessUs_) maximumLatenessUs_ = lateness;
        const std::uint32_t elapsedPeriods = lateness / periodUs + 1;
        missedTicks_ += elapsedPeriods - 1;
        deadlineUs_ += elapsedPeriods * periodUs;
        ++ticks_;
        return true;
    }

    [[nodiscard]] std::uint32_t slack(std::uint32_t nowUs, std::uint32_t periodUs) const {
        if (!initialized_) return 0;
        const auto remaining = static_cast<std::int32_t>(deadlineUs_ - nowUs);
        return remaining > 0 ? static_cast<std::uint32_t>(remaining) : 0;
    }

    [[nodiscard]] std::uint32_t ticks() const { return ticks_; }
    [[nodiscard]] std::uint32_t missedTicks() const { return missedTicks_; }
    [[nodiscard]] std::uint32_t maximumLatenessUs() const { return maximumLatenessUs_; }

private:
    std::uint32_t deadlineUs_ = 0;
    std::uint32_t ticks_ = 0;
    std::uint32_t missedTicks_ = 0;
    std::uint32_t maximumLatenessUs_ = 0;
    bool initialized_ = false;
};

} // namespace realtime

#endif
