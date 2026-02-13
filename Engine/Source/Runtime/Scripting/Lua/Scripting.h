#pragma once
#include "Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Memory/SmartPtr.h"
#include "Scripting/ScriptTypes.h"
#include "sol/sol.hpp"
#include "Tools/Actions/DeferredActions.h"
#include "World/Entity/Components/Component.h"


namespace Lumina
{
    class CStruct;
}




namespace Lumina::Scripting
{
    DECLARE_MULTICAST_DELEGATE(FScriptTransactionDelegate, FStringView);
    
    
    struct FLuaScriptMetadata;
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
        RUNTIME_API void ScriptReloaded(FStringView ScriptPath);
        RUNTIME_API void ScriptCreated(FStringView ScriptPath);
        RUNTIME_API void ScriptRenamed(FStringView NewPath, FStringView OldPath);
        RUNTIME_API void ScriptDeleted(FStringView ScriptPath);
        RUNTIME_API TSharedPtr<FLuaScript> LoadUniqueScript(FStringView Path);
        RUNTIME_API TVector<TSharedPtr<FLuaScript>> GetAllRegisteredScripts();
        RUNTIME_API void RunGC();
        
        
        void RegisterCoreTypes();
        void SetupInput();
        
        
        FScriptTransactionDelegate OnScriptLoaded;
        FScriptTransactionDelegate OnScriptDeleted;

    private:
        
        void ReloadScripts(FStringView Path);
        
        void Lua_Info(const sol::variadic_args& Args);
        void Lua_Warning(const sol::variadic_args& Args);
        void Lua_Error(const sol::variadic_args& Args);
    
    private:
        
        FSharedMutex SharedMutex;
        sol::state State;
        FDeferredActionRegistry DeferredActions;
        
        THashMap<FName, TVector<TWeakPtr<FLuaScript>>> RegisteredScripts;
    };
    
}

namespace sol
{
    template <typename T, typename Allocator>
    struct is_container<eastl::vector<T, Allocator>> : std::true_type {};

    template <typename T>
    struct is_container<Lumina::TVector<T>> : std::true_type {};
    
}