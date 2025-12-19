#pragma once

#include "Platform/GenericPlatform.h"
#include "Core/Object/ObjectMacros.h"
#include "RayCast.generated.h"

namespace Lumina
{
    REFLECT()
    struct FRayResult
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        int64 BodyID;
        
        PROPERTY(Script)
        uint32 Entity;
        
        PROPERTY(Script)
        glm::vec3 Start;
        
        PROPERTY(Script)
        glm::vec3 End;
        
        PROPERTY(Script)
        glm::vec3 Location;
        
        PROPERTY(Script)
        glm::vec3 Normal;
        
        PROPERTY(Script)
        float Fraction;
    };
    
    struct FRayCastSettings
    {
        glm::vec3 Start;
        glm::vec3 End;
        bool bDrawDebug;
        uint32 LayerMask;
        int64 IgnoreBody;
    };
}
