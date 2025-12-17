#pragma once

#include "Memory/Memory.h"
#include "Module/API.h"
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
        LUMINA_API void* AllocateCObject(uint32 Size, uint32 Alignment);

        /** Free memory of an object */
        LUMINA_API void FreeCObject(CObjectBase* Ptr);

    private:
        
    };
    
    extern LUMINA_API FCObjectAllocator GCObjectAllocator;

}
