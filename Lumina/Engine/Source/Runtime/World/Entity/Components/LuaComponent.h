#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Core/UpdateStage.h"
#include "sol/sol.hpp"
#include "Scripting/ScriptTypes.h"
#include "LuaComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct LUMINA_API SLuaComponent
    {
        GENERATED_BODY()
        

        FName                   TypeName;
        sol::table              LuaTable;
    };

    struct LUMINA_API FLuaScriptsContainerComponent
    {
        TArray<TVector<Scripting::FLuaSystemScriptEntry>, (uint32)EUpdateStage::Max>    Systems;
    };
}
