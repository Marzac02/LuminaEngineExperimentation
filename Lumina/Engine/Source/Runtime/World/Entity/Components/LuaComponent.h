#pragma once
#include "Core/Object/ObjectMacros.h"
#include "sol/sol.hpp"
#include "Component.h"
#include "Scripting/ScriptTypes.h"
#include "LuaComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SLuaComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SLuaComponent)
        

        FName                   TypeName;
        sol::table              LuaTable;
    };

    struct LUMINA_API FLuaScriptsContainerComponent
    {
        TArray<TVector<Scripting::FLuaSystemScriptEntry>, (uint32)EUpdateStage::Max> Systems;
    };
}
