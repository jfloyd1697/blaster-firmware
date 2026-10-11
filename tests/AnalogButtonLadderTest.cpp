#include <cassert>

#include "core/input/AnalogButtonLadder.h"

int main() {
    // A default-constructed ladder must honor the shared configuration struct.
    const AnalogButtonLadderConfig defaults{};
    const AnalogButtonLadder defaultLadder(defaults);
    assert(defaultLadder.config().nextMaximum == defaults.nextMaximum);
    assert(defaultLadder.config().previousMaximum == defaults.previousMaximum);
    assert(defaultLadder.config().reloadMaximum == defaults.reloadMaximum);
    assert(defaultLadder.buttonForValue(defaults.nextMaximum - 1) == ButtonID::NextWeapon);
    assert(defaultLadder.buttonForValue(defaults.nextMaximum) == ButtonID::PreviousWeapon);
    assert(defaultLadder.buttonForValue(defaults.previousMaximum) == ButtonID::Reload);
    assert(!defaultLadder.buttonForValue(defaults.reloadMaximum).has_value());

    // Hardware-specific calibration travels through the struct without overrides.
    const AnalogButtonLadder calibrated({245, 850, 2700});
    assert(calibrated.buttonForValue(90) == ButtonID::NextWeapon);
    assert(calibrated.buttonForValue(400) == ButtonID::PreviousWeapon);
    assert(calibrated.buttonForValue(1300) == ButtonID::Reload);
    assert(!calibrated.buttonForValue(4095).has_value());

    const AnalogButtonLadder ladder({400, 800, 3200});
    assert(ladder.buttonForValue(0) == ButtonID::NextWeapon);
    assert(ladder.buttonForValue(399) == ButtonID::NextWeapon);
    assert(ladder.buttonForValue(400) == ButtonID::PreviousWeapon);
    assert(ladder.buttonForValue(799) == ButtonID::PreviousWeapon);
    assert(ladder.buttonForValue(800) == ButtonID::Reload);
    assert(ladder.buttonForValue(3199) == ButtonID::Reload);
    assert(!ladder.buttonForValue(3200).has_value());
    return 0;
}
