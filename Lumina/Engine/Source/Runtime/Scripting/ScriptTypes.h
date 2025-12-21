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
        sol::environment    Environment;
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
