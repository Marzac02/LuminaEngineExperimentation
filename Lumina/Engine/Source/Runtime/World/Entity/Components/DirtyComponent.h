#pragma once

#include "Module/API.h"
#include "Physics/PhysicsTypes.h"

namespace Lumina
{
    enum class EMoveMode;

    struct LUMINA_API FNeedsTransformUpdate
    {
        EMoveMode MoveMode = EMoveMode::Teleport;
        bool bActivate = true;
    };
    
    struct LUMINA_API FNeedsPhysicsBodyUpdate
    {
        bool bFoobar;
    };
}
