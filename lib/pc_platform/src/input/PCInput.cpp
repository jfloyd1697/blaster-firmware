#include "platform/pc/input/PCInput.h"

#include <windows.h>

#include "core/input/IInput.h"

PCInput::PCInput(ITime* time)
    : IInput(time) {
}

bool PCInput::readRawButton(const ButtonID button) const {
    switch (button) {
        case ButtonID::Trigger:
            return ((GetAsyncKeyState(VK_SPACE) | GetAsyncKeyState(VK_RETURN)) & 0x8000) != 0;

        case ButtonID::NextWeapon:
            return ((GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('E')) & 0x8000) != 0;

        case ButtonID::PreviousWeapon:
            return ((GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('Q')) & 0x8000) != 0;

        case ButtonID::Reload:
            return (GetAsyncKeyState('R') & 0x8000) != 0;

        case ButtonID::Quit:
            return (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    }

    return false;
}

