#pragma once
#include "Core/Object/ObjectMacros.h"
#include "EntitySystem.h"
#include "UpdateTransformEntitySystem.generated.h"

namespace Lumina
{
    REFLECT()
    class LUMINA_API CUpdateTransformEntitySystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PostPhysics, EUpdatePriority::Highest), RequiresUpdate(EUpdateStage::Paused))
    public:

        
        void Update(FSystemContext& SystemContext) override;
        
    };
}
