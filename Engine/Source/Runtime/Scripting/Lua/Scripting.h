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
        struct FScriptLoad
        {
            FScriptLoad(FStringView Str)
                : Path(Str)
            {}
            
            FString Path;
        };
        
        struct FScriptDelete
        {
            FScriptDelete(FStringView Str)
                : Path(Str)
            {}
            
            FString Path;
        };
        
        struct FScriptRename
        {
            FScriptRename(FStringView A, FStringView B)
                : NewName(A), OldName(B)
            {}
            
            FString NewName;
            FString OldName;
        };
        
    public:

        RUNTIME_API static FScriptingContext& Get();

        RUNTIME_API sol::state_view GetState() { return sol::state_view(State); }

        void Initialize();
        void Shutdown();
        
        void ProcessDeferredActions();

        RUNTIME_API size_t GetScriptMemoryUsage() const;
        RUNTIME_API void OnScriptReloaded(FStringView ScriptPath);
        RUNTIME_API void OnScriptCreated(FStringView ScriptPath);
        RUNTIME_API void OnScriptRenamed(FStringView NewPath, FStringView OldPath);
        RUNTIME_API void OnScriptDeleted(FStringView ScriptPath);
        RUNTIME_API void LoadScripts(FStringView Directory);
        
        
        void RegisterCoreTypes();
        void SetupInput();

        template<typename TScript, typename TFunc>
        void ForEachScript(TFunc&& Func);
        
        FScriptReloadedDelegate OnScriptLoaded;


    private:
        
        void Lua_Info(const sol::variadic_args& Args);
        void Lua_Warning(const sol::variadic_args& Args);
        void Lua_Error(const sol::variadic_args& Args);

        TVector<entt::entity> LoadScript(FStringView ScriptData, bool bFailSilently = false);
    
    private:
        
        FSharedMutex SharedMutex;
        
        sol::state State;
        
        FDeferredActionRegistry DeferredActions;
        entt::registry ScriptRegistry;
        
        THashMap<FStringView, TVector<entt::entity>> PathToScriptEntities;
        
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