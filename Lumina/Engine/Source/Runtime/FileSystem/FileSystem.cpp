#include "pch.h"
#include "FileSystem.h"

#include <ranges>

#include "Core/Templates/LuminaTemplate.h"
#include "Paths/Paths.h"


namespace Lumina::FileSystem
{
    static THashMap<FName, TVector<FFileSystem>> FileSystemStorage;
    
    
    namespace Detail
    {
        template<typename TFunc>
        auto VisitFileSystems(FStringView Path, TFunc&& Func) -> decltype(Func(eastl::declval<FFileSystem&>()))
        {
            for (auto& [Alias, FileSystems] : FileSystemStorage)
            {
                if (!Path.starts_with(Alias.c_str()))
                {
                    continue;
                }
    
                for (FFileSystem& FileSystem : std::ranges::reverse_view(FileSystems))
                {
                    if (auto Result = Func(FileSystem))
                    {
                        return Result;
                    }
                }
            }
            
            return {};
        }
    }
    
    
    
    bool FFileSystem::ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(Result, Path); }, Storage);
    }

    bool FFileSystem::ReadFile(FString& OutString, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(OutString, Path); }, Storage);
    }

    bool FFileSystem::WriteFile(FStringView Path, FStringView Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    bool FFileSystem::WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    bool FFileSystem::Exists(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Exists(Path); }, Storage);
    }

    bool FFileSystem::CreateDir(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.CreateDir(Path); }, Storage);
    }

    bool FFileSystem::Remove(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Remove(Path); }, Storage);
    }

    bool FFileSystem::RemoveAll(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.RemoveAll(Path); }, Storage);
    }

    bool FFileSystem::Rename(FStringView Old, FStringView New) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Rename(Old, New); }, Storage);
    }

    FStringView FFileSystem::GetAliasPath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetAliasPath(); }, Storage);
    }

    FStringView FFileSystem::GetBasePath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetBasePath(); }, Storage);
    }

    FStringView Extension(FStringView Path)
    {
        size_t Dot = Path.find_last_of('.');
        if (Dot == FString::npos)
        {
            return {};
        }

        return Path.substr(Dot);
    }

    FStringView FileName(FStringView Path, bool bRemoveExtension)
    {
        size_t LastSlash = Path.find_last_of("/\\");
        if (LastSlash == FString::npos)
        {
            return {};
        }
        
        FStringView FilePart = Path.substr(LastSlash + 1);

        if (bRemoveExtension)
        {
            size_t DotPos = FilePart.find_last_of('.');
            if (DotPos != FString::npos)
            {
                return FilePart.substr(0, DotPos);
            }
        }

        return Move(FilePart);
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
        return {};
    }

    bool CreateDir(FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.CreateDir(Path))
            {
                return true;
            }
            
            return false;
        });
        
        return VisitResult;
    }

    bool IsUnderDirectory(FStringView Parent, FStringView Path)
    {
        if (!Path.starts_with(Parent))
        {
            return false;
        }
    
        return Path.length() == Parent.length() || 
               Path[Parent.length()] == '/' || 
               Path[Parent.length()] == '\\';
    }

    bool IsDirectory(FStringView Path)
    {
        return std::filesystem::is_directory(Path.data());
    }

    bool IsLuaAsset(FStringView Path)
    {
        return Extension(Path) == ".lua";
    }

    bool IsLuminaAsset(FStringView Path)
    {
        return Extension(Path) == ".lasset";
    }

    FStringView Parent(FStringView Path, bool bRemoveTrailingSlash)
    {
        size_t Pos = Path.find_last_of('/');
        if (Pos == FString::npos)
        {
            return {};
        }
        
        return Path.substr(0, bRemoveTrailingSlash ? Pos : Pos + 1);
    }

    bool ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                if (FS.ReadFile(Result, Path))
                {
                    return true;
                }
            }
            
            return false;
        });
        
        return VisitResult;
    }

    bool ReadFile(FString& OutString, FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                if (FS.ReadFile(OutString, Path))
                {
                    return true;
                }
            }
            
            return false;
        });
        
        return VisitResult;   
    }

    bool WriteFile(FStringView Path, FStringView Data)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.WriteFile(Path, Data))
            {
                return true;
            }
            return false;
        });
        
        return VisitResult;
    }

    bool WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.WriteFile(Path, Data))
            {
                return true;
            }
            return false;
        });
        
        return VisitResult;
    }

    bool Exists(FStringView Path)
    {
        bool Result = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                return true;
            }
            
            return false;
        });
        
        return Result;
    }

    bool CreateFile(FStringView Path)
    {
        return std::filesystem::create_directory(Path.data());
    }

    FFileSystem& Detail::AddFileSystemImpl(const FName& Alias, FFileSystem&& System)
    {
        return FileSystemStorage[Alias].emplace_back(Move(System));
    }

    TVector<FFileSystem>* Detail::GetFileSystems(const FName& Alias)
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
        Path = Paths::Normalize(Path);
        Ext = Paths::Normalize(Ext);
        size_t Dot = Path.find_last_of('.');
        if (Dot == FString::npos)
        {
            return false;
        }

        FStringView ActualExt = Path.substr(Dot);
        return ActualExt == Ext;
    }
}
