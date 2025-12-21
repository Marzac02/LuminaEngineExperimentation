#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Component.h"
#include "Physics/PhysicsTypes.h"
#include "PhysicsComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SRigidBodyComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SRigidBodyComponent)
        
        PROPERTY(Script, Visible, Category = "Physics")
        uint32 BodyID = UINT32_MAX;
        
        PROPERTY(Script, Editable, Category = "Physics")
        float Mass = 1.0f;

        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float LinearDamping = 0.0f;

        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float AngularDamping = 0.05f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        EBodyType BodyType = EBodyType::Dynamic;
        
        PROPERTY(Script, Editable, Category = "Physics")
        bool bUseGravity = true;
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