#include "pch.h"
#include "ScriptSystem.h"

#include "World/World.h"
#include "World/Entity/Components/ScriptComponent.h"
#include "World/Entity/Events/WorldEvents.h"


namespace Lumina
{
    static void NotifyScriptsBeginPlay(const FBeginPlayEvent& Event)
    {
        auto View = Event.World->GetEntityRegistry().view<SScriptComponent>();
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
            
                Script->Environment["Entity"] = Entity;
                Script->Environment["Context"] = std::ref(Event.World->GetSystemContext());
            
                if (sol::optional<sol::function> BeginPlayFunc = Script->ScriptTable["BeginPlay"])
                {
                    sol::protected_function_result Result = (*BeginPlayFunc)(Script->ScriptTable);
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        });
    }
    
    static void NotifyScriptsEndPlay(const FEndPlayEvent& Event)
    {
        auto View = Event.World->GetEntityRegistry().view<SScriptComponent>();
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
            
                Script->Environment["Entity"] = Entity;
                Script->Environment["Context"] = std::ref(Event.World->GetSystemContext());
            
                if (sol::optional<sol::function> BeginPlayFunc = Script->ScriptTable["EndPlay"])
                {
                    sol::protected_function_result Result = (*BeginPlayFunc)(Script->ScriptTable);
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        });
    }
    
    void SScriptSystem::Startup(const FSystemContext& Context) noexcept
    {
        Context.EventSink<FBeginPlayEvent>().connect<&NotifyScriptsBeginPlay>();
        Context.EventSink<FEndPlayEvent>().connect<&NotifyScriptsEndPlay>();
    }

    void SScriptSystem::Update(const FSystemContext& Context) noexcept
    {
        LUMINA_PROFILE_SCOPE(); 
        
        auto View = Context.CreateView<SScriptComponent>();
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
            
                Script->Environment["Entity"] = Entity;
                Script->Environment["Context"] = std::ref(Context);
                
                if (sol::optional<sol::function> BeginPlayFunc = Script->ScriptTable["Update"])
                {
                    sol::protected_function_result Result = (*BeginPlayFunc)(Script->ScriptTable, Context.GetDeltaTime());
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        });
    }

    void SScriptSystem::Teardown(const FSystemContext& Context) noexcept
    {
        auto View = Context.CreateView<SScriptComponent>();
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
            
                Script->Environment["Entity"] = Entity;
                Script->Environment["Context"] = std::ref(Context);
            
                if (sol::optional<sol::function> BeginPlayFunc = Script->ScriptTable["Teardown"])
                {
                    sol::protected_function_result Result = (*BeginPlayFunc)(Script->ScriptTable);
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        });
        
        Context.EventSink<FBeginPlayEvent>().disconnect<&NotifyScriptsBeginPlay>();
        Context.EventSink<FEndPlayEvent>().disconnect<&NotifyScriptsEndPlay>();
    }
}
