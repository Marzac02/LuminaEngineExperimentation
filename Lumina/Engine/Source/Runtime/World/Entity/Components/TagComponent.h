#pragma once

#include "TagComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct LUMINA_API STagComponent
    {
        GENERATED_BODY()

        PROPERTY(Script)
        FName Tag;
    };
}
