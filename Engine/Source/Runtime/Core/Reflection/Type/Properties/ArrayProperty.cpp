#include "pch.h"
#include "ArrayProperty.h"

namespace Lumina
{
    void FArrayProperty::Serialize(FArchive& Ar, void* Value)
    {
        SIZE_T ElementCount = GetNum(Value);
        Ar << ElementCount;
        if (ElementCount > eastl::numeric_limits<uint32>::max())
        {
            LOG_ERROR("Array Property tried to serialize {} elements. Aborted", ElementCount);
            return;
        }

        if (Ar.IsWriting())
        {
            for (SIZE_T i = 0; i < ElementCount; i++)
            {
                Inner->Serialize(Ar, GetAt(Value, i));
            }
        }
        else
        {
            for (SIZE_T i = 0; i < ElementCount; ++i)
            {
                PushBack(Value, nullptr);
                Inner->Serialize(Ar, GetAt(Value, i));
            }
        }
    }

    void FArrayProperty::SerializeItem(IStructuredArchive::FSlot Slot, void* Value, void const* Defaults)
    {
        UNREACHABLE();
    }
}
