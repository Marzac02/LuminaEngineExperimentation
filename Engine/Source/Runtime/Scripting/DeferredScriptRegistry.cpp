#include "pch.h"
#include "DeferredScriptRegistry.h"

#include "Log/Log.h"

namespace Lumina::Scripting
{
    FDeferredScriptRegistry& FDeferredScriptRegistry::Get()
    {
        static FDeferredScriptRegistry Instance;
        return Instance;
    }

    void FDeferredScriptRegistry::ProcessRegistrations(const sol::state_view& State)
    {
        for (auto Fn : PendingRegistrations)
        {
            Fn(State);
        }

        PendingRegistrations.clear();
        PendingRegistrations.shrink_to_fit();
    }

    void FDeferredScriptRegistry::AddPending(void(* Fn)(sol::state_view))
    {
        PendingRegistrations.push_back(Fn);
    }
}
