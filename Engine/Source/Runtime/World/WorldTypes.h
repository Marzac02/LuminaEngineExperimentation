#pragma once

#include "WorldTypes.generated.h"
#include "Core/Object/ObjectMacros.h"

namespace Lumina
{

    REFLECT()
    enum class EWorldType : uint8
    {
        None,
        Game,
        Editor,
        EditorPreview,
        Inactive,
    };
    
}
