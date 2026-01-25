#pragma once

#include "EntitySystem.h"
#include "CharacterMovementSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CCharacterMovementSystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics, EUpdatePriority::Highest))
        
    public:
        
        void Update(FSystemContext& SystemContext) override;
    };
}
