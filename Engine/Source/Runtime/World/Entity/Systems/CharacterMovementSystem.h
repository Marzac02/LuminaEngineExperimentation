#pragma once

#include "EntitySystem.h"
#include "CharacterMovementSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SCharacterMovementSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics, EUpdatePriority::Highest))
        
    public:
        
        static void Update(const FSystemContext& SystemContext) noexcept;
    };
}
