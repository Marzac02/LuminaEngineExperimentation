#pragma once

namespace Lumina
{
    class IRenderContext;
    
    struct FRHIGlobals
    {
        bool bRHIInitialized = false;
    };


    RUNTIME_API extern FRHIGlobals GRHIGlobals;
    RUNTIME_API extern IRenderContext* GRenderContext;

#define GIsRHIInitialized GRHIGlobals.bRHIInitialized

}
