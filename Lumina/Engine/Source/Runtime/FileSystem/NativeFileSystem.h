#pragma once
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Containers/Array.h"

namespace Lumina::FileSystem
{
    class LUMINA_API FNativeFileSystem
    {
    public:
        FNativeFileSystem(const FName& InAliasPath, const FString& InBasePath) noexcept;
        
        
        bool ReadFile(TVector<uint8>& Result, FStringView Path);
        bool ReadFile(FString& OutString, FStringView Path);
        
        bool WriteFile(FStringView Path, FStringView Data);
        bool WriteFile(FStringView Path, TSpan<const uint8> Data);
    
        FStringView GetAliasPath() const { return AliasPath; }
        FStringView GetBasePath() const { return BasePath; }
        
    private:
        
        FString AliasPath;
        FString BasePath;
    };
}
