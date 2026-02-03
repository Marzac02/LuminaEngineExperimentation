#pragma once
#include "Core/Object/ObjectMacros.h"
#include "EntitySystem.h"
#include "UpdateTransformEntitySystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SUpdateTransformEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PostPhysics, EUpdatePriority::Highest), RequiresUpdate(EUpdateStage::Paused))
    public:

        
        static void Update(const FSystemContext& SystemContext) noexcept;
        
    };
}
