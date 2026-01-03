#pragma once

#include <glm/glm.hpp>
#include "VelocityComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SVelocityComponent
    {
        GENERATED_BODY()

        PROPERTY(ReadOnly)
        glm::vec3 Velocity;

        PROPERTY(Editable)
        float Speed;
    };
    
}
