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
}
