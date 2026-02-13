#include "pch.h"
#include "ScriptSystem.h"
#include "World/Entity/Components/ScriptComponent.h"


namespace Lumina
{
    void SScriptSystem::Startup(const FSystemContext& Context) noexcept
    {
        
    }

    void SScriptSystem::Update(const FSystemContext& Context) noexcept
    {
        LUMINA_PROFILE_SCOPE(); 
        
        auto View = Context.CreateView<SScriptComponent>();
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script  = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
                
                Script->Environment["Entity"]   = Entity;
                Script->Environment["Context"]  = std::ref(Context);
                sol::protected_function_result Result = Script->ScriptTable["Update"](Script->ScriptTable, Context.GetDeltaTime());
                if (!Result.valid())
                {
                    sol::error Error = Result;
                    LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                }
            }
        });
    }

    void SScriptSystem::Teardown(const FSystemContext& Context) noexcept
    {
    }
}
