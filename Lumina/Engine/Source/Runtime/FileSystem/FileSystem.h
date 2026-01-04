#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"
#include "Memory/SmartPtr.h"

namespace Lumina::FileSystem
{
    class LUMINA_API IFileSystem
    {
    public:
        
        IFileSystem() = default;
        virtual ~IFileSystem() = default;
        IFileSystem(IFileSystem&&) = delete;
        IFileSystem(const IFileSystem&) = delete;
        IFileSystem& operator=(const IFileSystem&) = delete;
        IFileSystem& operator=(IFileSystem&&) = delete;
        
        
        
        virtual bool ReadFile(TVector<uint8>& Result, FStringView Path, uint32 ReadFlags = 0) = 0;
        virtual bool ReadFile(FString& OutString, FStringView Path, uint32 ReadFlags = 0) = 0;
        
        virtual bool WriteFile(FStringView Path, TSpan<const uint8> Data) = 0;
        virtual bool WriteFile(FStringView Path, FStringView Data) = 0;
    };
}
