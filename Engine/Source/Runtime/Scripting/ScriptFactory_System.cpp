#include "pch.h"
#include "ScriptFactory_System.h"
#include "World/Entity/Components/Component.h"
#include "Core/UpdateStage.h"

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
        
        EUpdatePriority Priority    = (EUpdatePriority)ScriptTable["Priority"].get_or(0);
        EUpdateStage Stage          = (EUpdateStage)ScriptTable["Stage"].get_or(0);
        
        FLuaSystemScriptEntry Entry;
        Entry.Name          = Name;
        Entry.PriorityList.SetStagePriority(FUpdateStagePriority(Stage, Priority));
        Entry.InitFunc      = ScriptTable["Init"];
        Entry.ExecuteFunc   = ScriptTable["Execute"];
        Entry.ShutdownFunc  = ScriptTable["Shutdown"];
        
        ScriptRegistry.emplace<FLuaSystemScriptEntry>(ScriptEntity, Entry);
        
        return ScriptEntity;
    }
}
