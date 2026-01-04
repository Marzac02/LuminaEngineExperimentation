#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"
#include "Memory/SmartPtr.h"
#include "NativeFileSystem.h"
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
    
    class LUMINA_API FAnyFileSystem
    {
    public:
        
        template<CFileSystem T>
        FAnyFileSystem(T&& FS) 
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
        requires(eastl::is_constructible_v<T, FName, TArgs...>)
        friend T* Mount(const FName& Alias, TArgs&&... Args);
        
        template<CFileSystem T>
        friend T* GetFileSystem(const FName& Alias);
    };
    
    static void Initialize();
    static void Shutdown();
    
    namespace Detail
    {
        LUMINA_API FAnyFileSystem* AddFileSystemImpl(const FName& Alias, FAnyFileSystem&& System);
        LUMINA_API FAnyFileSystem* GetFileSystemImpl(const FName& Alias);
    }
    
    template<CFileSystem T, typename... TArgs>
    requires(eastl::is_constructible_v<T, FName, TArgs...>)
    T* Mount(const FName& Alias, TArgs&&... Args)
    {
        T TypeT(Alias, Forward<TArgs>(Args)...);
        FAnyFileSystem FS(TypeT);
        
        FAnyFileSystem* Result = Detail::AddFileSystemImpl(Alias, Move(FS));
        return Result ? eastl::get_if<T>(&Result->Storage) : nullptr;
    }
    
    template<CFileSystem T>
    T* GetFileSystem(const FName& Alias)
    {
        if (FAnyFileSystem* FS = Detail::GetFileSystemImpl(Alias))
        {
            return eastl::get_if<T>(&FS->Storage);
        }
        return nullptr;
    }
    
    LUMINA_API FStringView FileName(FStringView Path);
    LUMINA_API bool Remove(FStringView Path);
    LUMINA_API bool RemoveAll(FStringView Path);
    LUMINA_API FFixedString ResolvePath(FStringView Path);
    LUMINA_API FStringView GetMountPath(const FName& Alias);
    LUMINA_API FAnyFileSystem* GetFileSystem(const FName& Alias);
    LUMINA_API bool DoesAliasExists(const FName& Alias);
    
    LUMINA_API bool ReadFile(TVector<uint8>& Result, FStringView Path);
    LUMINA_API bool ReadFile(FString& OutString, FStringView Path);
    LUMINA_API bool WriteFile(FStringView Path, FStringView Data);
    LUMINA_API bool WriteFile(FStringView Path, TSpan<const uint8> Data);
    
    LUMINA_API bool HasExtension(FStringView Path, FStringView Ext);
}
