#include "pch.h"
#include "PropertyMetadata.h"

namespace Lumina
{
    void FMetaDataPair::AddValue(const FName& Key, const FName& Value)
    {
        PairParams.emplace(Key, Value);
    }

    bool FMetaDataPair::HasMetadata(const FName& Key) const
    {
        return PairParams.find(Key) != PairParams.end();
    }

    const FName& FMetaDataPair::GetMetadata(const FName& Key) const
    {
        return PairParams.at(Key);
    }
}
