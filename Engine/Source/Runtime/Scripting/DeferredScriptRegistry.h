#pragma once
#include "Containers/Array.h"
#include <sol/sol.hpp>

namespace Lumina::Scripting
{
    class FDeferredScriptRegistry
    {
    public:
        
        RUNTIME_API static FDeferredScriptRegistry& Get();
        void ProcessRegistrations(const sol::state_view& State);
        RUNTIME_API void AddPending(void (*Fn) (sol::state_view));

        
        TVector<void(*)(sol::state_view)> PendingRegistrations;
    };


    struct RUNTIME_API FRegisterScriptInfo
    {
        FRegisterScriptInfo(void(*Fn)(sol::state_view))
        {
            FDeferredScriptRegistry::Get().AddPending(Fn);
        }
    };
}
