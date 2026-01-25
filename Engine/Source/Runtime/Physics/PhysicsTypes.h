#pragma once

#include "Core/Object/ObjectMacros.h"
#include "PhysicsTypes.generated.h"

namespace Lumina
{
    REFLECT()
    enum class RUNTIME_API EMoveMode : uint8
    {
        Teleport,           // Hard set position (loses velocity)
        MoveKinematic,      // Move with velocity calculation (preserves physics)
        ActivateOnly        // Just wake up, don't move
    };
    
    REFLECT()
    enum class RUNTIME_API EBodyType : uint8
    {
        None,
        Static,
        Kinematic,
        Dynamic,
    };    
}
