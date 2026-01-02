#pragma once

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "sol/sol.hpp"
#include "Core/Delegates/Delegate.h"
#include "World/Entity/Components/LuaComponent.h"
#include "scriptentitysystem.generated.h"



namespace Lumina
{
    REFLECT()
    class CScriptEntitySystem : public CEntitySystem
    {
        GENERATED_BODY()
        
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::FrameStart),
            RequiresUpdate(EUpdateStage::PrePhysics),
            RequiresUpdate(EUpdateStage::DuringPhysics),
            RequiresUpdate(EUpdateStage::PostPhysics),
            RequiresUpdate(EUpdateStage::FrameEnd),
            RequiresUpdate(EUpdateStage::Paused))
        
    public:

        void Init(FSystemContext& SystemContext) override;
        void Update(FSystemContext& SystemContext) override;
        void Shutdown(FSystemContext& SystemContext) override;
        
    };
}
