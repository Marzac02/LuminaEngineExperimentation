#pragma once
#include "EntitySystem.h"
#include "EditorEntityMovementSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CEditorEntityMovementSystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::Paused, EUpdatePriority::Highest))
    public:
        

        void Update(FSystemContext& SystemContext) override;
        
    };
}
