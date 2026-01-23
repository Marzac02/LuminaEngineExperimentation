#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Containers/Name.h"
#include "NameComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct LUMINA_API SNameComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        FName Name;
    };
}
