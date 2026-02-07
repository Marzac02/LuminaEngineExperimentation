#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Scripting/ScriptPath.h"
#include "Memory/SmartPtr.h"
#include "ScriptComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SScriptComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable)
        FScriptPath ScriptPath;
        
        FScriptCustomData CustomData;
        TSharedPtr<Scripting::FLuaScript> Script;
    };
}
