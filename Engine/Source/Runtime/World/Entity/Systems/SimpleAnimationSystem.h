#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "SimpleAnimationSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SSimpleAnimationSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics), RequiresUpdate(EUpdateStage::Paused))

    public:

        static void Update(const FSystemContext& SystemContext) noexcept;
    
    };
}
