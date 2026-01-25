#pragma once

#include "Physics/PhysicsTypes.h"

namespace Lumina
{
    enum class EMoveMode : uint8;

    struct RUNTIME_API FNeedsTransformUpdate
    {
        EMoveMode MoveMode = EMoveMode::Teleport;
        bool bActivate = true;
    };
    
    struct RUNTIME_API FNeedsPhysicsBodyUpdate
    {
        bool bFoobar;
    };
}
