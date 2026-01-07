#pragma once

#include "SameAs.h"
#include "Containers/Array.h"

namespace Lumina
{
    enum class Byte : uint8;

    template<typename T>
    TSpan<const Byte> AsBytes(TSpan<T> Type) noexcept
    {
        return TSpan<const Byte>(reinterpret_cast<const Byte*>(Type.data()), Type.size_bytes());
    }
    
}
