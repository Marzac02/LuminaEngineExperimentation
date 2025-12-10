#pragma once

#include "Component.h"
#include "Core/Object/ObjectMacros.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "Physics/Physics.h"
#include "CharacterComponent.generated.h"


namespace Lumina
{
    REFLECT()
    struct LUMINA_API SCharacterPhysicsComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SCharacterPhysicsComponent)
    
        // Jolt character reference
        JPH::Ref<JPH::CharacterVirtual> Character;
    
        // Physics properties
        PROPERTY(Script, Editable, Category = "Collision")
        float HalfHeight = 1.8f;

        PROPERTY(Script, Editable, Category = "Collision")
        float Radius = 1.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float Mass = 70.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float MaxStrength = 100.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float MaxSlopeAngle = 45.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float StepHeight = 0.4f;
    };

    REFLECT()
    struct LUMINA_API SCharacterMovementComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SCharacterMovementComponent)
    
        // Movement parameters
        PROPERTY(Script, Editable, Category = "Movement")
        float MoveSpeed = 5.0f;

        PROPERTY(Script, Editable, Category = "Movement")
        float Acceleration = 10.0f;

        PROPERTY(Script, Editable, Category = "Movement")
        float Deceleration = 8.0f;

        PROPERTY(Script, Editable, Category = "Movement")
        float AirControl = 0.3f;

        PROPERTY(Script, Editable, Category = "Movement")
        float GroundFriction = 8.0f;
    
        PROPERTY(Script, Editable, Category = "Movement")
        float JumpSpeed = 8.0f;
    
        PROPERTY(Script, Editable, Category = "Movement")
        int MaxJumpCount = 1;
    
        PROPERTY(Script, Editable, Category = "Gravity")
        float Gravity = Physics::GEarthGravity;

        PROPERTY(Script, Visible, Category = "Movement")
        glm::vec3 Velocity;
        
        bool bGrounded = false;
        int JumpCount = 0;
    };

    
}
