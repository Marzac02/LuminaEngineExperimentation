#pragma once
#include "World/Entity/Components/Component.h"
#include "Core/Object/ObjectMacros.h"
#include "ImpulseEvent.generated.h"

namespace Lumina
{
    REFLECT()
    struct SImpulseEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SImpulseEvent)
        
        PROPERTY(Script)
        uint32 BodyID;
        
        PROPERTY(Script)
        glm::vec3 Impulse;
    };
    
    REFLECT()
    struct SForceEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SForceEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    };

    REFLECT()
    struct STorqueEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(STorqueEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Torque;
    };

    REFLECT()
    struct SAngularImpulseEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SAngularImpulseEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularImpulse;
    };

    REFLECT()
    struct SSetVelocityEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SSetVelocityEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Velocity;
    };

    REFLECT()
    struct SSetAngularVelocityEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SSetAngularVelocityEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularVelocity;
    };

    REFLECT()
    struct SAddImpulseAtPositionEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SAddImpulseAtPositionEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Impulse;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT()
    struct SAddForceAtPositionEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SAddForceAtPositionEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT()
    struct SSetGravityFactorEvent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SSetGravityFactorEvent)
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        float GravityFactor;
    };
}
