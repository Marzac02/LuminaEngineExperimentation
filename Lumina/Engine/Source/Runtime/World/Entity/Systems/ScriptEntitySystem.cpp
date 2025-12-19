#include "pch.h"
#include "ScriptEntitySystem.h"

#include "World/Entity/Components/LuaComponent.h"


namespace Lumina
{
    void CScriptEntitySystem::RegisterEventListeners(FSystemContext& SystemContext)
    {

    }
    
    void CScriptEntitySystem::Update(FSystemContext& SystemContext)
    {
        LUMINA_PROFILE_SCOPE();
        
        auto View = SystemContext.CreateView<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& ScriptContainer)
        {
            for (const Scripting::FLuaSystemScriptEntry* Entry : ScriptContainer.Systems[(uint32)SystemContext.GetUpdateStage()])
            {
                entt::runtime_view RuntimeView = SystemContext.CreateRuntimeView(Entry->Queries);
                std::vector<entt::entity> Entities(RuntimeView.begin(), RuntimeView.end());

                if (Entities.empty())
                {
                    continue;
                }
                
                try
                {
                    Entry->ExecuteFunc(SystemContext, Entities, SystemContext.GetDeltaTime());
                }
                catch (sol::error& Error)
                {
                    LOG_ERROR("Script Error: {0}", Error.what());
                }
            }
        });
    }
}
