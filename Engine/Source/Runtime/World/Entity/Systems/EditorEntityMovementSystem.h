#pragma once
#include "EntitySystem.h"
#include "EditorEntityMovementSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SEditorEntityMovementSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::Paused, EUpdatePriority::Highest))
    public:
        

        static void Update(const FSystemContext& SystemContext) noexcept;
        
    };
}
