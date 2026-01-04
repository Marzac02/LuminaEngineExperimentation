#pragma once
#include "FileSystem.h"

namespace Lumina::FileSystem
{
    class LUMINA_API FNativeFileSystem : public IFileSystem
    {
    public:
        
        FNativeFileSystem(const FName& InAliasPath, const FString& InBasePath) noexcept;
        
        FNativeFileSystem(const FNativeFileSystem&) = delete;
        FNativeFileSystem(FNativeFileSystem&&) = delete;
        
        FNativeFileSystem& operator=(const FNativeFileSystem&) = delete;
        FNativeFileSystem& operator=(FNativeFileSystem&&) = delete;
        
        
        bool ReadFile(TVector<uint8>& Result, FStringView Path, uint32 ReadFlags) override;
        bool ReadFile(FString& OutString, FStringView Path, uint32 ReadFlags) override;
        
        bool WriteFile(FStringView Path, FStringView Data) override;
        bool WriteFile(FStringView Path, TSpan<const uint8> Data) override;
    
    private:
        
        FString AliasPath;
        FString BasePath;
    };
}
