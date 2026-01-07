#pragma once

#include "SameAs.h"
#include "Containers/Array.h"

namespace Lumina
{
    enum class Byte : uint8;

    template<typename T>
    requires Concept::TSameAs<typename eastl::iterator_traits<typename T::iterator>::iterator_category, eastl::random_access_iterator_tag>
    TSpan<Byte> AsBytes(const T& Type)
    {
        auto ViewSpan = TSpan(Type);
        return TSpan<Byte>(reinterpret_cast<const Byte*>(Type.data()), Type.size_bytes());
    }
    
}
