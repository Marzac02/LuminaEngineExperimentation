#pragma once
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class CObjectBase;
}

namespace Lumina
{
    class FCObjectAllocator
    {
    public:

        FCObjectAllocator();
        ~FCObjectAllocator();

        /** Allocates memory for a new CObject, but does not place in memory */
        RUNTIME_API void* AllocateCObject(uint32 Size, uint32 Alignment);

        /** Free memory of an object */
        RUNTIME_API void FreeCObject(CObjectBase* Ptr);

    private:
        
    };
    
    extern RUNTIME_API FCObjectAllocator GCObjectAllocator;

}
