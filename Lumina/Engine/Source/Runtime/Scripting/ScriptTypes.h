#pragma once

#include "Containers/String.h"
#include "Platform/GenericPlatform.h"


namespace Lumina::Scripting
{
    struct FLuaScriptMetadata
    {
        FName Name;
        FName Author;
        FName Version;
        FName Description;
    };
    
    struct FLuaScriptEntry
    {
        FString             Path;
        sol::environment    Environment;
        FName               Type;

        template<typename T>
        const T& As() const
        {
            return *static_cast<const T*>(this);
        }
    };

    struct FLuaSystemScriptEntry : FLuaScriptEntry
    {
        FName                   Name;
        int                     Stage;
        int                     Priority;
        TVector<entt::id_type>  Queries;
        sol::protected_function ExecuteFunc;
    };
}
