#include "pch.h"
#include "PropertyMetadata.h"

namespace Lumina
{
    void FMetaDataPair::AddValue(const FName& Key, const FName& Value)
    {
        PairParams.emplace(Key, Value);
    }

    bool FMetaDataPair::HasMetadata(const FName& Key)
    {
        return PairParams.find(Key) != PairParams.end();
    }

    FName FMetaDataPair::GetMetadata(const FName& Key)
    {
        return PairParams.at(Key);
    }
}
