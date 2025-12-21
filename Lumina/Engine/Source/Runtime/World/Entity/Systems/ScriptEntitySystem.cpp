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
            for (const Scripting::FLuaSystemScriptEntry& Entry : ScriptContainer.Systems[(uint32)SystemContext.GetUpdateStage()])
            {
                entt::runtime_view RuntimeView = SystemContext.CreateRuntimeView(Entry.Queries);
                std::vector<entt::entity> Entities(RuntimeView.begin(), RuntimeView.end());

                if (Entities.empty())
                {
                    continue;
                }

                if (sol::protected_function_result Result = Entry.ExecuteFunc(std::ref(SystemContext), std::ref(Entities), SystemContext.GetDeltaTime()); !Result.valid())
                {
                    sol::error Error = Result;
                    LOG_ERROR("Script Error in system '{0}': {1}", Entry.Name, Error.what());
                }
            }
        });
    }
}
