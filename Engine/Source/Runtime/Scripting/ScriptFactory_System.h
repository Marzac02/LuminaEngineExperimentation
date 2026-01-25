#pragma once
#include "Core/Object/ObjectMacros.h"
#include "ScriptFactory.h"
#include "ScriptFactory_System.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CScriptFactory_System : public CScriptFactory
    {
        GENERATED_BODY()
        
    public:
        entt::entity ProcessScript(FName Name, const sol::table& ScriptTable, entt::registry& ScriptRegistry) const override;
    };
}
