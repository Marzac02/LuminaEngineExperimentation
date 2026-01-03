#pragma once
#include "Core/Object/ObjectMacros.h"
#include "SpringArmComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct SSpringArmComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        float TargetArmLength = 3.0f;

        PROPERTY(Editable)
        glm::vec3 SocketOffset;

        PROPERTY(Editable)
        float ProbeSize = 0.2f;

        PROPERTY(Editable)
        bool bDoCollisionTest = true;

        PROPERTY(Editable)
        bool bUseControlRotation = true;
    
    };
}
