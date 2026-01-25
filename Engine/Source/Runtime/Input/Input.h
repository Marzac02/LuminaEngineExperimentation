#pragma once

#include "Platform/GenericPlatform.h"

namespace Lumina::Input
{
    enum class EKeyState : uint8
    {
        Up = 0,
        Down = 1,
        Repeated = 2,
    };

    enum class EMouseState : uint8
    {
        Up = 0,
        Down = 1,
    };

    enum class EInputMode : uint8
    {
        Normal = 0,
        Hidden,
        Disabled
    };

    enum class EInputDevice : uint8
    {
        Keyboard = 0,
        Mouse,
        Gamepad,
        Touch
    };
}