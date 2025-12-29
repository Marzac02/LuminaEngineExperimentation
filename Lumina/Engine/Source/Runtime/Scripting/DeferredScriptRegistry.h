#pragma once
#include "Containers/Array.h"
#include <sol/sol.hpp>
#include "Scripting/ScriptTypes.h"
#include "Module/API.h"

namespace Lumina::Scripting
{
    class FDeferredScriptRegistry
    {
    public:
        
        LUMINA_API static FDeferredScriptRegistry& Get();
        void ProcessRegistrations(const sol::state_view& State);
        LUMINA_API void AddPending(void (*Fn) (sol::state_view));

        
        TVector<void(*)(sol::state_view)> PendingRegistrations;
    };


    struct LUMINA_API FRegisterScriptInfo
    {
        FRegisterScriptInfo(void(*Fn)(sol::state_view))
        {
            FDeferredScriptRegistry::Get().AddPending(Fn);
        }
    };
}
