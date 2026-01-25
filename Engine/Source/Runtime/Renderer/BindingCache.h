#pragma once
#include "RenderResource.h"
#include "Containers/Array.h"
#include "Core/Threading/Thread.h"

namespace Lumina
{
    struct FBindingSetDesc;
}

namespace Lumina
{
    struct FBindingLayoutDesc;
}

namespace Lumina
{
    class FBindingCache
    {
    public:

        FRHIBindingLayout* GetOrCreateBindingLayout(const FBindingLayoutDesc& Desc);
        FRHIBindingSet* GetOrCreateBindingSet(const FBindingSetDesc& Desc, FRHIBindingLayout* Layout);

        
        void ReleaseResources();

    private:

        FMutex Mutex;
        THashMap<size_t, FRHIBindingLayoutRef> BindingLayouts;
        THashMap<size_t, FRHIBindingSetRef> BindingSets;

    };
}
