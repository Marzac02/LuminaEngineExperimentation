#pragma once

#include "TagComponent.generated.h"

namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API STagComponent
    {
        GENERATED_BODY()

        PROPERTY(Script)
        FName Tag;
    };
}
