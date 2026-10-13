#include <cassert>
#include <cstdint>
#include "realtime/ControlSchedule.h"

int main() {
    realtime::ControlSchedule schedule;
    assert(schedule.due(100, 1000));
    assert(!schedule.due(1099, 1000));
    assert(schedule.slack(1099, 1000) == 1);
    assert(schedule.due(1100, 1000));
    assert(schedule.due(4500, 1000));
    assert(schedule.missedTicks() == 2);
    assert(schedule.maximumLatenessUs() == 2400);
    realtime::ControlSchedule wrap;
    assert(wrap.due(UINT32_MAX - 100, 1000));
    assert(!wrap.due(100, 1000));
    assert(wrap.due(899, 1000));
}
