#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Component.h"
#include "PhysicsComponent.generated.h"

namespace Lumina
{

    REFLECT()
    enum class LUMINA_API EBodyType : uint8
    {
        None,
        Static,
        Kinematic,
        Dynamic,
    };    
    
    
    REFLECT()
    struct LUMINA_API SRigidBodyComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SRigidBodyComponent)

        PROPERTY(Editable, Category = "Physics")
        EBodyType BodyType = EBodyType::Dynamic;
        
        PROPERTY(Visible, Category = "Physics")
        uint64 BodyID = UINT64_MAX;
        
        PROPERTY(Editable, Category = "Physics")
        float Mass = 1.0f;

        PROPERTY(Editable, Category = "Physics")
        bool bUseGravity = true;

        PROPERTY(Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float LinearDamping = 0.0f;

        PROPERTY(Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float AngularDamping = 0.05f;
    };

    REFLECT()
    struct LUMINA_API SBoxColliderComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SBoxColliderComponent)

        PROPERTY(Editable)
        glm::vec3 HalfExtent = glm::vec3(0.5f);

        PROPERTY(Editable)
        glm::vec3 Offset;
    };

    REFLECT()
    struct LUMINA_API SSphereColliderComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SSphereColliderComponent)

        PROPERTY(Editable)
        float Radius = 0.5f;

        PROPERTY(Editable)
        glm::vec3 Offset;
    };
    
}