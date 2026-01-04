#pragma once
#include "Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "sol/sol.hpp"
#include "Tools/Actions/DeferredActions.h"
#include "World/Entity/Components/Component.h"


namespace Lumina
{
    class CStruct;
}

DECLARE_MULTICAST_DELEGATE(FScriptReloadedDelegate);


namespace Lumina::Scripting
{
    void Initialize();
    void Shutdown();
    
    class FScriptingContext
    {
    
        struct FScriptReload
        {
            FString Path;
        };
        
    public:

        LUMINA_API static FScriptingContext& Get();

        LUMINA_API sol::state_view GetState() { return sol::state_view(State); }

        void Initialize();
        void Shutdown();
        
        void ProcessDeferredActions();

        LUMINA_API SIZE_T GetScriptMemoryUsage() const;
        LUMINA_API void OnScriptReloaded(FStringView ScriptPath);
        LUMINA_API void OnScriptCreated(FStringView ScriptPath);
        LUMINA_API void OnScriptRenamed(FStringView NewPath, FStringView OldPath);
        LUMINA_API void OnScriptDeleted(FStringView ScriptPath);
        LUMINA_API void LoadScriptsInDirectoryRecursively(FStringView Directory);
        
        
        void RegisterCoreTypes();
        void SetupInput();

        template<typename TScript, typename TFunc>
        void ForEachScript(TFunc&& Func);
        
        FScriptReloadedDelegate OnScriptLoaded;


    private:
        
        void Lua_Print(const sol::variadic_args& Args);
        void Lua_Info(const sol::variadic_args& Args);
        void Lua_Warning(const sol::variadic_args& Args);
        void Lua_Error(const sol::variadic_args& Args);

        TVector<entt::entity> LoadScriptPath(FStringView ScriptPath, bool bFailSilently = false);
    
    private:
        FMutex LoadMutex;
        
        sol::state State;
        
        FDeferredActionRegistry DeferredActions;
        entt::registry ScriptRegistry;
        
        THashMap<FName, TVector<entt::entity>> PathToScriptEntities;
        
    };

    template <typename TScript, typename TFunc>
    void FScriptingContext::ForEachScript(TFunc&& Func)
    {
        ScriptRegistry.view<TScript>().each(Forward<TFunc>(Func));
    }
}

namespace sol
{
    template <typename T, typename Allocator>
    struct is_container<eastl::vector<T, Allocator>> : std::true_type {};

    template <typename T>
    struct is_container<Lumina::TVector<T>> : std::true_type {};
    
}