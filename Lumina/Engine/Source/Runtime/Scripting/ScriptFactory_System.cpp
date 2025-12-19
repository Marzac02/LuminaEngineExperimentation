#include "pch.h"
#include "ScriptFactory_System.h"

#include "World/Entity/Components/Component.h"

namespace Lumina
{
    CScriptFactory::FScriptExpected CScriptFactory_System::ProcessScript(FName Name, const sol::table& ScriptTable) const
    {
        sol::object TypeObj = ScriptTable["Type"];
        if (!TypeObj.valid() || !TypeObj.is<const char*>())
        {
            return TUnexpected("Missing or invalid Type field");
        }
    
        FName Type = TypeObj.as<const char*>();
    
        if (Type != "System")
        {
            return TUnexpected("Not a System type");
        }
        
        auto Entry = MakeUniquePtr<Scripting::FLuaSystemScriptEntry>();
        Entry->Name         = Name;
        Entry->Type         = "System";
        Entry->Priority     = ScriptTable["Priority"].get_or(0);
        Entry->Stage        = ScriptTable["Stage"].get_or(0);
        Entry->ExecuteFunc  = ScriptTable["Execute"].get_or<sol::function>(sol::nil);

        if (sol::optional<sol::table> QueryTable = ScriptTable["Query"])
        {
            for (entt::id_type IDType : ECS::CollectTypes(QueryTable.value()))
            {
                Entry->Queries.emplace_back(IDType);
            }
        }
        
        return Entry;
    }
}
