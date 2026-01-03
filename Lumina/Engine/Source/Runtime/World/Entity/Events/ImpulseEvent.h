#pragma once

#include "Core/Object/ObjectMacros.h"
#include "ImpulseEvent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct SImpulseEvent
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        uint32 BodyID;
        
        PROPERTY(Script)
        glm::vec3 Impulse;
    };
    
    REFLECT(Component)
    struct SForceEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    };

    REFLECT(Component)
    struct STorqueEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Torque;
    };

    REFLECT(Component)
    struct SAngularImpulseEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularImpulse;
    };

    REFLECT(Component)
    struct SSetVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Velocity;
    };

    REFLECT(Component)
    struct SSetAngularVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularVelocity;
    };

    REFLECT(Component)
    struct SAddImpulseAtPositionEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Impulse;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT(Component)
    struct SAddForceAtPositionEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT(Component)
    struct SSetGravityFactorEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        float GravityFactor;
    };
}
