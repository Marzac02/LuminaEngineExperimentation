#pragma once
#include "Component.h"
#include "TagComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API STagComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(STagComponent)

        PROPERTY(Script)
        FName Tag;
    };
}
