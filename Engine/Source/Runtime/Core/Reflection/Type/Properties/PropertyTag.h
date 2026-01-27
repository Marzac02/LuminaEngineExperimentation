#pragma once
#include "Lumina.h"
#include "Containers/Name.h"


namespace Lumina
{
    class FProperty;
}

namespace Lumina
{
    /** Aids in serialization of reflected properties. */
    class FPropertyTag
    {
    public:
        
        /** Type of the property */
        FName Type;

        /** Name of the property */
        FName Name;

        /** Size of the property */
        int32 Size = 0;

        /** Offset into the buffer in bytes */
        int64 Offset = 0;
        
        
        friend FArchive& operator << (FArchive& Ar, FPropertyTag& Data)
        {
            Ar << Data.Type;
            Ar << Data.Name;
            Ar << Data.Size;
            Ar << Data.Offset;

            return Ar;
        }
    };
    
    struct FPropertyTagTable
    {
        uint32 Version = 1;
        uint32 NumEntries = 0;
        
        // Followed by: FPropertyTagEntry[NumEntries]
        // Followed by: Raw property data
    };
}
