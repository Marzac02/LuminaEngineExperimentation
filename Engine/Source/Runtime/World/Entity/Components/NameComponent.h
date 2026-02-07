#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Containers/Name.h"
#include "NameComponent.generated.h"

namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API SNameComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        FName Name;
    };
}
