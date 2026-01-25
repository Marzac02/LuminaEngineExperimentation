#include "pch.h"
#include "ScriptEntitySystem.h"

#include "Scripting/ScriptTypes.h"
#include "World/Entity/Components/LuaComponent.h"


namespace Lumina
{
    void CScriptEntitySystem::Init(FSystemContext& SystemContext)
    {
        auto View = SystemContext.CreateView<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& ScriptContainer)
        {
            for (const TVector<Scripting::FLuaSystemScriptEntry>& Scripts : ScriptContainer.Systems)
            {
                for (const Scripting::FLuaSystemScriptEntry& Script : Scripts)
                {
                    if (sol::protected_function_result Result = Script.InitFunc(std::ref(SystemContext)); !Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error in system '{0}': {1}", Script.Name, Error.what());
                    }
                }
            }
        });
    }
    
    void CScriptEntitySystem::Update(FSystemContext& SystemContext)
    {
        LUMINA_PROFILE_SCOPE();
        
        auto View = SystemContext.CreateView<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& ScriptContainer)
        {
            for (const Scripting::FLuaSystemScriptEntry& Entry : ScriptContainer.Systems[(uint32)SystemContext.GetUpdateStage()])
            {
                if (sol::protected_function_result Result = Entry.ExecuteFunc(std::ref(SystemContext), SystemContext.GetDeltaTime()); !Result.valid())
                {
                    sol::error Error = Result;
                    LOG_ERROR("Script Error in system '{0}': {1}", Entry.Name, Error.what());
                }
            }
        });
    }
    
    void CScriptEntitySystem::Shutdown(FSystemContext& SystemContext)
    {
        auto View = SystemContext.CreateView<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& ScriptContainer)
        {
            for (const TVector<Scripting::FLuaSystemScriptEntry>& Scripts : ScriptContainer.Systems)
            {
                for (const Scripting::FLuaSystemScriptEntry& Script : Scripts)
                {
                    //if (sol::protected_function_result Result = Script.ShutdownFunc(std::ref(SystemContext)); !Result.valid())
                    //{
                    //    sol::error Error = Result;
                    //    LOG_ERROR("Script Error in system '{0}': {1}", Script.Name, Error.what());
                    //}
                }
            }
        });
    }
}
