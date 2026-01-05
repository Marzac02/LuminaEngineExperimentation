#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"
#include "Memory/SmartPtr.h"
#include "NativeFileSystem.h"
#include "Containers/Function.h"
#include "Core/Functional/FunctionRef.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Core/Templates/SameAs.h"
#include "Core/Variant/Variant.h"

namespace Lumina::FileSystem
{
    template<typename T>
    concept CFileSystem = requires(T FS, TVector<uint8>& OutBytes, FString& OutStr, 
                                   FStringView Path, TSpan<const uint8> Data)
    {
        { FS.ReadFile(OutBytes, Path) }         -> Concept::TSameAs<bool>;
        { FS.ReadFile(OutStr, Path) }           -> Concept::TSameAs<bool>;
        { FS.WriteFile(Path, Path) }            -> Concept::TSameAs<bool>;
        { FS.WriteFile(Path, Data) }            -> Concept::TSameAs<bool>;
        { FS.GetAliasPath() }                   -> std::convertible_to<FStringView>;
        { FS.GetBasePath() }                    -> std::convertible_to<FStringView>;
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
        
        FStringView GetAliasPath() const;
        FStringView GetBasePath() const;
        
    private:
        
        TVariant<FNativeFileSystem> Storage;
        
        ////
        
        template<CFileSystem T, typename... TArgs>
        requires(eastl::is_nothrow_constructible_v<T, FName, TArgs...>)
        friend T& Mount(const FName& Alias, TArgs&&... Args);
        
        template<CFileSystem T>
        friend T* GetFileSystem(const FName& Alias);
    };
    
    static void Initialize();
    static void Shutdown();
    
    namespace Detail
    {
        LUMINA_API FFileSystem& AddFileSystemImpl(const FName& Alias, FFileSystem&& System);
    }
    
    template<CFileSystem T, typename... TArgs>
    requires(eastl::is_nothrow_constructible_v<T, FName, TArgs...>)
    T& Mount(const FName& Alias, TArgs&&... Args)
    {
        T TypeT(Alias, Forward<TArgs>(Args)...);
        FFileSystem FS(TypeT);
        
        FFileSystem& Result = Detail::AddFileSystemImpl(Alias, Move(FS));
        return eastl::get<T>(Result.Storage);
    }
    
    
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
    LUMINA_API FStringView Parent(FStringView Path);
    
    LUMINA_API bool ReadFile(TVector<uint8>& Result, FStringView Path);
    LUMINA_API bool ReadFile(FString& OutString, FStringView Path);
    LUMINA_API bool WriteFile(FStringView Path, FStringView Data);
    LUMINA_API bool WriteFile(FStringView Path, TSpan<const uint8> Data);
    
    LUMINA_API bool HasExtension(FStringView Path, FStringView Ext);
    
    void DirectoryIterator(FStringView Path, const TFunctionRef<void(FStringView Path)>& Func);
    void ForEachFileSystem(const TFunctionRef<void(FFileSystem&)>& Func);
    
}
