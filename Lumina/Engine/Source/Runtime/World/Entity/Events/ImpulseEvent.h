#pragma once

#include "Core/Object/ObjectMacros.h"
#include "ImpulseEvent.generated.h"

namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct SImpulseEvent
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        uint32 BodyID;
        
        PROPERTY(Script)
        glm::vec3 Impulse;
    };
    
    REFLECT(Component, HideInComponentList)
    struct SForceEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    };

    REFLECT(Component, HideInComponentList)
    struct STorqueEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Torque;
    };

    REFLECT(Component, HideInComponentList)
    struct SAngularImpulseEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularImpulse;
    };

    REFLECT(Component, HideInComponentList)
    struct SSetVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Velocity;
    };

    REFLECT(Component, HideInComponentList)
    struct SSetAngularVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularVelocity;
    };

    REFLECT(Component, HideInComponentList)
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

    REFLECT(Component, HideInComponentList)
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

    REFLECT(Component, HideInComponentList)
    struct SSetGravityFactorEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        float GravityFactor;
    };
}
