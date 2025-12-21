#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Core/Reflection/Type/Properties/StructProperty.h"
#include "Memory/SmartPtr.h"
#include "Scripting/ScriptTypes.h"
#include "sol/sol.hpp"
#include "World/Entity/Components/Component.h"


namespace Lumina
{
    class CStruct;
}

DECLARE_MULTICAST_DELEGATE(FScriptReloadedDelegate);

namespace Lumina::Scripting
{
    struct FLuaConverter
    {
        using ToFunctionType = sol::object(const sol::state_view&, void*);
        using FromFunctionType = void*(const sol::object&);
        
        sol::object (*To) (const sol::state_view&, void*);
        void* (*From) (const sol::object&);

        void (*ConnectLuaHandler)(entt::dispatcher&, const sol::function&);
    };
    

    void Initialize();
    void Shutdown();

    using FScriptEntryHandle = TUniquePtr<FLuaScriptEntry>;
    using FScriptHashMap = THashMap<FName, TVector<FScriptEntryHandle>>;
    
    class FScriptingContext
    {
    public:

        LUMINA_API static FScriptingContext& Get();

        LUMINA_API sol::state_view GetState() { return sol::state_view(State); }

        void Initialize();
        void SetPanicHandler();
        void Shutdown();

        LUMINA_API SIZE_T GetScriptMemoryUsage() const;
        LUMINA_API void OnScriptReloaded(FStringView ScriptPath);
        LUMINA_API void OnScriptCreated(FStringView ScriptPath);
        LUMINA_API void OnScriptRenamed(FStringView NewPath, FStringView OldPath);
        LUMINA_API void OnScriptDeleted(FStringView ScriptPath);
        LUMINA_API void LoadScriptsInDirectoryRecursively(FStringView Directory);
        LUMINA_API const TVector<FScriptEntryHandle>& GetScriptsUnderDirectory(FName Directory);
        
        
        void RegisterCoreTypes();
        void SetupInput();
        sol::object ConvertToSolObjectFromName(FName Name, const sol::state_view& View, void* Data);
        FLuaConverter::ToFunctionType* GetSolConverterFunctionPtrByName(FName Name);
        
        void* ConvertFromSolObjectToVoidPtrByName(FName Name, const sol::object& Object);


        template<typename TFunc>
        void ForEachScript(FName Type, TFunc&& Func);
        
        template<typename T>
        bool RegisterLuaConverterByName(FName Name);
        
        
        FScriptReloadedDelegate OnScriptLoaded;


    private:

        TVector<TUniquePtr<FLuaScriptEntry>> LoadScript(FStringView ScriptPath, bool bFailSilently = false);
    
    private:
        
        FMutex Mutex;
        
        sol::state State;

        THashMap<FName, FLuaConverter> LuaConverters;

        FScriptHashMap RegisteredScripts;
        
    };


    template <typename TFunc>
    void FScriptingContext::ForEachScript(FName Type, TFunc&& Func)
    {
        if (Type != NAME_None)
        {
            auto It = RegisteredScripts.find(Type);
            if (It == RegisteredScripts.end())
            {
                return;
            }
            
            for (FScriptEntryHandle& LuaScriptEntry : It->second)
            {
                eastl::invoke(Func, *LuaScriptEntry.get());
            }

            return;
        }
        
        for (auto& [_, ScriptVector] : RegisteredScripts)
        {
            for (FScriptEntryHandle& LuaScriptEntry : ScriptVector)
            {
                eastl::invoke(Func, *LuaScriptEntry.get());
            }
        }
    }
    
    template <typename T>
    bool FScriptingContext::RegisterLuaConverterByName(FName Name)
    {
        if (LuaConverters.find(Name) != LuaConverters.end())
        {
            LOG_ERROR("Lua Converter Already Registered for type {}", Name);
            return false;
        }
        
        LuaConverters.emplace(Name, FLuaConverter{+[](const sol::state_view& L, void* Data)
        {
            T& Ref = *static_cast<T*>(Data);
            return sol::make_object(L, std::ref(Ref));
        },
        +[](const sol::object& Obj)
        {
            return (void*)&Obj.as<T>();
        },
        +[](entt::dispatcher& Dispatcher, const sol::function& Fn)
        {
            //Dispatcher.sink<T>() ///....
        }});
        
        return true;
    }
}
