#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "SimpleAnimationSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CSimpleAnimationSystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics), RequiresUpdate(EUpdateStage::Paused))

    public:

        void Update(FSystemContext& SystemContext) override;
    
    };
}
