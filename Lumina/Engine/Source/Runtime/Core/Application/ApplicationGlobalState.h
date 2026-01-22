#pragma once

#include "Module/API.h"

namespace Lumina
{
    
    
    struct LUMINA_API FApplicationGlobalState
    {
        FApplicationGlobalState(char const* MainThreadName = nullptr);
        ~FApplicationGlobalState();
    };
}