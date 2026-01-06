#pragma once

#include "FileInfo.h"
#include "NativeFileSystem.h"
#include "Containers/Function.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Core/Templates/SameAs.h"
#include "Core/Variant/Variant.h"

namespace Lumina::FileSystem
{
    template<typename T>
    concept CFileSystem = requires(T FS,    TVector<uint8>& OutBytes, FString& OutStr, 
                                            FStringView Path, TSpan<const uint8> Data,
                                            const TFunction<void(const FFileInfo&)>& Callback)
    {
        { FS.ReadFile(OutBytes, Path) }                         -> Concept::TSameAs<bool>;
        { FS.ReadFile(OutStr, Path) }                           -> Concept::TSameAs<bool>;
        { FS.WriteFile(Path, Path) }                            -> Concept::TSameAs<bool>;
        { FS.WriteFile(Path, Data) }                            -> Concept::TSameAs<bool>;
        { FS.Exists(Path) }                                     -> Concept::TSameAs<bool>;
        { FS.CreateDir(Path) }                                  -> Concept::TSameAs<bool>;
        { FS.Remove(Path) }                                     -> Concept::TSameAs<bool>;
        { FS.RemoveAll(Path) }                                  -> Concept::TSameAs<bool>;
        { FS.Rename(Path, Path) }                               -> Concept::TSameAs<bool>;
        { FS.DirectoryIterator(Path, Callback) }                -> Concept::TSameAs<void>;
        { FS.RecursiveDirectoryIterator(Path, Callback) }       -> Concept::TSameAs<void>;
        { FS.GetAliasPath() }                                   -> std::convertible_to<FStringView>;
        { FS.GetBasePath() }                                    -> std::convertible_to<FStringView>;
    };
    
    class LUMINA_API FFileSystem
    {
    public:
        
        template<CFileSystem T>
        explicit FFileSystem(T&& FS) 
            : Storage(Move(FS)) 
        {}
        
        bool ReadFile(TVector<uint8>& Result, FStringView Path);
        bool ReadFile(FString& OutString, FStringView Path);
        bool WriteFile(FStringView Path, FStringView Data);
        bool WriteFile(FStringView Path, TSpan<const uint8> Data);
        bool Exists(FStringView Path) const;
        bool IsDirectory(FStringView Path) const;
        bool CreateDir(FStringView Path) const;
        bool Remove(FStringView Path) const;
        bool RemoveAll(FStringView Path) const;
        bool Rename(FStringView Old, FStringView New) const;
        void DirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const;
        void RecursiveDirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const;

        
        FStringView GetAliasPath() const;
        FStringView GetBasePath() const;
        
    private:
        
        TVariant<FNativeFileSystem> Storage;
        
        ////
        
        template<CFileSystem T, typename... TArgs>
        requires(eastl::is_nothrow_constructible_v<T, FFixedString, TArgs...>)
        friend T& Mount(const FFixedString& Alias, TArgs&&... Args);
    };
    
    static void Initialize();
    static void Shutdown();
    
    namespace Detail
    {
        LUMINA_API FFileSystem& AddFileSystemImpl(const FFixedString& Alias, FFileSystem&& System);
        LUMINA_API TVector<FFileSystem>* GetFileSystems(const FFixedString& Alias);
    }
    
    template<CFileSystem T, typename... TArgs>
    requires(eastl::is_nothrow_constructible_v<T, FFixedString, TArgs...>)
    T& Mount(const FFixedString& Alias, TArgs&&... Args)
    {
        T TypeT(Alias, Forward<TArgs>(Args)...);
        FFileSystem FS(TypeT);
        
        FFileSystem& Result = Detail::AddFileSystemImpl(Alias, Move(FS));
        return eastl::get<T>(Result.Storage);
    }
    
    LUMINA_API void DirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback);
    LUMINA_API void RecursiveDirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback);

    LUMINA_API FStringView RemoveExtension(FStringView Path);
    LUMINA_API bool Rename(FStringView Old, FStringView New);
    LUMINA_API FFixedString MakeUniqueFilePath(FStringView BasePath);
    LUMINA_API FStringView Extension(FStringView Path);
    LUMINA_API FStringView FileName(FStringView Path, bool bRemoveExtension = false);
    LUMINA_API bool Remove(FStringView Path);
    LUMINA_API bool RemoveAll(FStringView Path);
    LUMINA_API FFixedString ResolvePath(FStringView Path);
    LUMINA_API bool DoesAliasExists(const FName& Alias);
    LUMINA_API bool CreateDir(FStringView Path);
    LUMINA_API bool IsUnderDirectory(FStringView Parent, FStringView Path);
    LUMINA_API bool IsDirectory(FStringView Path);
    LUMINA_API bool IsLuaAsset(FStringView Path);
    LUMINA_API bool IsLuminaAsset(FStringView Path);
    LUMINA_API FStringView Parent(FStringView Path, bool bRemoveTrailingSlash = false);
    
    LUMINA_API bool ReadFile(TVector<uint8>& Result, FStringView Path);
    LUMINA_API bool ReadFile(FString& OutString, FStringView Path);
    LUMINA_API bool WriteFile(FStringView Path, FStringView Data);
    LUMINA_API bool WriteFile(FStringView Path, TSpan<const uint8> Data);
    
    LUMINA_API bool Exists(FStringView Path);
    LUMINA_API bool CreateFile(FStringView Path);
    
    LUMINA_API bool HasExtension(FStringView Path, FStringView Ext);
    
}
