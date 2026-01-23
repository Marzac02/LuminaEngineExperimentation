#include "pch.h"
#include "Subsystem.h"

namespace Lumina
{
    FSubsystemManager::~FSubsystemManager()
    {
        DEBUG_ASSERT(SubsystemLookup.empty());
    }
}
