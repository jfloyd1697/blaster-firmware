#include <windows.h>

#include "platform/pc/input/PCInput.h"
#include "core/time/ITime.h"

PCInput::PCInput(ITime* time)
    : IInput(time) {
}

bool PCInput::readRawButton(const ButtonID button) const {
    switch (button) {
        case ButtonID::Trigger:
            return isKeyDown(VK_SPACE);

        case ButtonID::NextWeapon:
            return isKeyDown(VK_RIGHT);

        case ButtonID::PreviousWeapon:
            return isKeyDown(VK_LEFT);

        case ButtonID::Reload:
            return isKeyDown('R');

        case ButtonID::Quit:
            return isKeyDown(VK_ESCAPE);
    }

    return false;
}

bool PCInput::isKeyDown(const int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}
