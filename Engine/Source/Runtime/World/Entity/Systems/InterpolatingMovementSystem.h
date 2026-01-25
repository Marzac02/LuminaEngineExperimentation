#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "InterpolatingMovementSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CInterpolatingMovementSystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics, EUpdatePriority::Default))

    public:

        void Update(FSystemContext& SystemContext) override;
    
    };
}
