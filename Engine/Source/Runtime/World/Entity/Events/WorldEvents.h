#pragma once

namespace Lumina
{
    class CWorld;
    
    struct RUNTIME_API FBeginPlayEvent
    {
        CWorld* World = nullptr;
    };
    
    struct RUNTIME_API FEndPlayEvent
    {
        CWorld* World = nullptr;
    };
}