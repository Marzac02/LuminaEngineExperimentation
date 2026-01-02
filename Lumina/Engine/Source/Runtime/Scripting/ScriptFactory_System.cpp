#include "pch.h"
#include "ScriptFactory_System.h"

#include "World/Entity/Components/Component.h"

namespace Lumina
{
    entt::entity CScriptFactory_System::ProcessScript(FName Name, const sol::table& ScriptTable, entt::registry& ScriptRegistry) const
    {
        using namespace Scripting;
        
        sol::object TypeObj = ScriptTable["Type"];
        if (!TypeObj.valid() || !TypeObj.is<const char*>())
        {
            return entt::null;
        }
    
        FName Type = TypeObj.as<const char*>();
    
        if (Type != "System")
        {
            return entt::null;
        }
        
        entt::entity ScriptEntity = ScriptRegistry.create();

        FLuaSystemScriptEntry Entry;
        Entry.Name          = Name;
        Entry.Priority      = ScriptTable["Priority"].get_or(0);
        Entry.Stage         = ScriptTable["Stage"].get_or(0);
        Entry.InitFunc      = ScriptTable["Init"].get_or<sol::function>(sol::nil);
        Entry.ExecuteFunc   = ScriptTable["Execute"].get_or<sol::function>(sol::nil);
        Entry.ShutdownFunc  = ScriptTable["Shutdown"].get_or<sol::function>(sol::nil);

        if (sol::optional<sol::table> QueryTable = ScriptTable["Query"])
        {
            for (entt::id_type IDType : ECS::CollectTypes(QueryTable.value()))
            {
                Entry.Queries.emplace_back(IDType);
            }
        }
        
        ScriptRegistry.emplace<FLuaSystemScriptEntry>(ScriptEntity, Entry);
        
        return ScriptEntity;
    }
}
