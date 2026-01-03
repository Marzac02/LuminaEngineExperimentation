#pragma once

#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Module/API.h"

namespace Lumina
{
    class LUMINA_API FMetaDataPair
    {
    public:

        void AddValue(const FName& Key, const FName& Value);

        bool HasMetadata(const FName& Key) const;
        FName GetMetadata(const FName& Key) const;
    

    private:

        THashMap<FName, FName> PairParams;
    };
}
