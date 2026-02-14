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
        
        template<typename... TArgs>
        void InvokeScriptFunction(FStringView Name, TArgs&&... Args)
        {
            if (Script && Script->ScriptTable.valid())
            {
                if (sol::optional<sol::function> ScriptFunc = Script->ScriptTable[Name.data()])
                {
                    sol::protected_function_result Result = (*ScriptFunc)(Script->ScriptTable, Forward<TArgs>(Args)...);
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        }
        
        PROPERTY(Editable)
        FScriptPath ScriptPath;
        
        FScriptCustomData CustomData;
        TSharedPtr<Scripting::FLuaScript> Script;
    };
}
