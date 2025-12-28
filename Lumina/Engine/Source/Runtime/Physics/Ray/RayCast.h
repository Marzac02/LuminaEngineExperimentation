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
    
    REFLECT()
    struct FRayCastSettings
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        glm::vec3 Start = glm::vec3(0.0f);
        
        PROPERTY(Script)
        glm::vec3 End = glm::vec3(0.0f);
        
        PROPERTY(Script)
        bool bDrawDebug = false;
        
        PROPERTY(Script)
        float DebugDuration = 0.0f;
        
        PROPERTY(Script)
        glm::vec3 DebugHitColor = glm::vec3(0.0, 1.0f, 0.0f);
        
        PROPERTY(Script)
        glm::vec3 DebugMissColor = glm::vec3(1.0f, 0.0f, 0.0f);
        
        PROPERTY(Script)
        uint32 LayerMask;
        
        PROPERTY(Script)
        TVector<int64> IgnoreBodies;
    };
}
