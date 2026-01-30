#pragma once
#include "Platform/GenericPlatform.h"

namespace Lumina::ECS
{
    enum class ETraits : uint8
    {
        None,
        System,
        Component,
        Event,
    };
    
    ENUM_CLASS_FLAGS(ETraits);
}
