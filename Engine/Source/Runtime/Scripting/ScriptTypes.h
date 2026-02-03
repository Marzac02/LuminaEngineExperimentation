#pragma once

#include "Containers/String.h"
#include "Containers/Name.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/UpdateStage.h"

namespace sol 
{
    template <>
    struct lua_type_of<Lumina::FString> : std::integral_constant<type, type::string> {};
    
    template <>
    struct is_container<Lumina::FString> : std::false_type {};
    
    template <typename T>
    struct unique_usertype_traits<Lumina::TObjectPtr<T>>
    {
        typedef T type;
        typedef Lumina::TObjectPtr<T> actual_type;
        static constexpr bool value = true;

        static bool is_null(const actual_type& ptr) 
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr) 
        {
            return ptr.Get();
        }
    };
    
}

namespace Lumina::Scripting
{
    struct FLuaScriptMetadata
    {
        FString Name;
        FString Path;
        FString Author;
        FString Version;
        FString Description;
    };
    
    struct FLuaScriptEntry
    {
        sol::environment    Environment;
    };

    struct FLuaSystemScriptEntry : FLuaScriptEntry
    {
        FName                   Name;
        
        FUpdatePriorityList     PriorityList;
        
        sol::protected_function InitFunc;
        sol::protected_function ExecuteFunc;
        sol::protected_function ShutdownFunc;
    };
}
