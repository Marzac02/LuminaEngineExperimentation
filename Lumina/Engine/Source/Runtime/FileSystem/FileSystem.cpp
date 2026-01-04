#include "pch.h"
#include "FileSystem.h"

#include "Core/Templates/LuminaTemplate.h"


namespace Lumina::FileSystem
{
    static THashMap<FName, FAnyFileSystem> FileSystemStorage;
    
    namespace Detail
    {
        struct FResolvedPath
        {
            FAnyFileSystem* FileSystem;
            FStringView RelativePath;
            bool bValid;
        };
        
        static FResolvedPath ResolvePath(FStringView Path)
        {
            size_t DelimPos = Path.find("://");
            if (DelimPos != FStringView::npos)
            {
                FStringView AliasStr = Path.substr(0, DelimPos);
                FStringView RelPath = Path.substr(DelimPos + 3);
                
                FName Alias(AliasStr.data());
                if (FAnyFileSystem* FS = GetFileSystem(Alias))
                {
                    return { FS, RelPath, true };
                }
                
                return { nullptr, {}, false };
            }
            
            if (FAnyFileSystem* DefaultFS = GetFileSystem(FName("default")))
            {
                return {.FileSystem = DefaultFS, .RelativePath = Path, .bValid = true };
            }
            
            return { nullptr, {}, false };
        }
    }
    
    bool FAnyFileSystem::ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(Result, Path); }, Storage);
    }

    bool FAnyFileSystem::ReadFile(FString& OutString, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(OutString, Path); }, Storage);
    }

    bool FAnyFileSystem::WriteFile(FStringView Path, FStringView Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    bool FAnyFileSystem::WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    FStringView FAnyFileSystem::GetAliasPath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetAliasPath(); }, Storage);
    }

    FStringView FAnyFileSystem::GetBasePath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetBasePath(); }, Storage);
    }

    FStringView FileName(FStringView Path)
    {
        size_t Pos = Path.find_last_of('/');
        if (Pos == FStringView::npos)
        {
            return {};
        }
        
        return Path.substr(Pos + 1);
    }

    bool Remove(FStringView Path)
    {
        FFixedString BasePath = ResolvePath(Path);
        return std::filesystem::remove(BasePath.c_str());
    }

    bool RemoveAll(FStringView Path)
    {
        FFixedString BasePath = ResolvePath(Path);
        return std::filesystem::remove_all(BasePath.c_str());
    }

    FFixedString ResolvePath(FStringView Path)
    {
        FFixedString NativeFilePath;
        if (Path.starts_with(GetMountPath(Path.data())))
        {
            FStringView Remainder = Path.substr(Path.length());
            NativeFilePath.append(Path.data()).append("/").append(Remainder.data());
        }
        
        return NativeFilePath;
    }

    FStringView GetMountPath(const FName& Alias)
    {
        auto It = FileSystemStorage.find(Alias);
        if (It == FileSystemStorage.end())
        {
            return {};
        }
        
        return It->second.GetBasePath();
    }

    FAnyFileSystem* GetFileSystem(const FName& Alias)
    {
        auto It = FileSystemStorage.find(Alias);
        if (It == FileSystemStorage.end())
        {
            return nullptr;
        }
        
        return &It->second;
    }

    bool ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        Detail::FResolvedPath ResolvedPath = Detail::ResolvePath(Path);
        if (!ResolvedPath.bValid)
        {
            return false;
        }
        
        return ResolvedPath.FileSystem->ReadFile(Result, Path);
    }

    bool ReadFile(FString& OutString, FStringView Path)
    {
        Detail::FResolvedPath ResolvedPath = Detail::ResolvePath(Path);
        if (!ResolvedPath.bValid)
        {
            return false;
        }
        
        return ResolvedPath.FileSystem->ReadFile(OutString, Path);
    }

    bool WriteFile(FStringView Path, FStringView Data)
    {
        Detail::FResolvedPath ResolvedPath = Detail::ResolvePath(Path);
        if (!ResolvedPath.bValid)
        {
            return false;
        }
        
        return ResolvedPath.FileSystem->WriteFile(Path, Data);
    }

    bool WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        Detail::FResolvedPath ResolvedPath = Detail::ResolvePath(Path);
        if (!ResolvedPath.bValid)
        {
            return false;
        }
        
        return ResolvedPath.FileSystem->WriteFile(Path, Data);
    }
    
    FAnyFileSystem* Detail::AddFileSystemImpl(const FName& Alias, FAnyFileSystem&& System)
    {
        auto [It, bInserted] = FileSystemStorage.emplace(Alias, Move(System));
        if (!bInserted)
        {
            return nullptr;
        }
        
        return &It->second;
    }

    FAnyFileSystem* Detail::GetFileSystemImpl(const FName& Alias)
    {
        auto It = FileSystemStorage.find(Alias);
        if (It == FileSystemStorage.end())
        {
            return nullptr;
        }
        
        return &It->second;
    }
    
    bool HasExtension(FStringView Path, FStringView Ext)
    {
        size_t Dot = Path.find_last_of('.');
        if (Dot == FString::npos)
        {
            return false;
        }

        FStringView ActualExt = Path.substr(Dot);
        return StringUtils::ToLower(ActualExt) == StringUtils::ToLower(Ext);
    }
}
