#pragma once
#include "EntitySystem.h"
#include "CameraSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class CCameraSystem : public CEntitySystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(US_PostPhysics), RequiresUpdate(US_Paused))
    public:

        void RegisterEventListeners(FSystemContext& SystemContext) override;

        void NewCameraConstructed(entt::registry& Registry, entt::entity Entity);
        void Update(FSystemContext& SystemContext) override;
        
    
    };
}
