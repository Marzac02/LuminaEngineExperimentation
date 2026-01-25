#pragma once

#include "Containers/Array.h"
#include "Containers/String.h"

namespace Lumina::FileHelper
{

    RUNTIME_API bool SaveArrayToFile(const TVector<uint8>& Array, FStringView Path, uint32 WriteFlags = 0);
    RUNTIME_API bool LoadFileToArray(TVector<uint8>& Result, FStringView Path, uint32 ReadFlags = 0);
    RUNTIME_API bool LoadFileToArray(TVector<uint8>& Result, FStringView Path, uint32 Seek, uint32 ReadSize, uint32 ReadFlags = 0);
    RUNTIME_API FString FileFinder(const FString& FileName, FStringView IteratorPath, bool bRecursive = true);
    
    RUNTIME_API bool LoadFileIntoString(FString& OutString, FStringView Path, uint32 ReadFlags = 0);
    RUNTIME_API bool SaveStringToFile(FStringView String, FStringView Path, uint32 WriteFlags = 0);
    RUNTIME_API bool DoesDirectoryExist(FStringView FilePath);
    RUNTIME_API bool CreateNewFile(FStringView FilePath, bool bBinary = false, uint32 Flags = 0);
    RUNTIME_API uint64 GetFileSize(FStringView FilePath);
    
}
