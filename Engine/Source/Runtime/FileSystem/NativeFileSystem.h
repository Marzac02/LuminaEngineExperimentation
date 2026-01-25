#pragma once
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Containers/Array.h"
#include "Containers/Function.h"

namespace Lumina::FileSystem
{
    struct FFileInfo;

    class RUNTIME_API FNativeFileSystem
    {
    public:
        FNativeFileSystem(const FFixedString& InAliasPath, FStringView InBasePath) noexcept;
        
        FFixedString ResolveVirtualPath(FStringView Path) const;
        
        bool ReadFile(TVector<uint8>& Result, FStringView Path);
        bool ReadFile(FString& OutString, FStringView Path);
        
        bool WriteFile(FStringView Path, FStringView Data);
        bool WriteFile(FStringView Path, TSpan<const uint8> Data);
        
        bool Exists(FStringView Path) const;
        bool IsDirectory(FStringView Path) const;
        size_t Size(FStringView Path) const;

        bool CreateDir(FStringView Path) const;
        
        bool Remove(FStringView Path) const;
        bool RemoveAll(FStringView Path) const;
        
        bool Rename(FStringView Old, FStringView New) const;
        
        void DirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const;
        void RecursiveDirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const;

        
    
        FStringView GetAliasPath() const { return AliasPath; }
        FStringView GetBasePath() const { return BasePath; }
        
    private:
        
        FFixedString AliasPath;
        FFixedString BasePath;
    };
}
