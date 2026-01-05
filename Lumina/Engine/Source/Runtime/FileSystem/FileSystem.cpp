#include "pch.h"
#include "FileSystem.h"

#include "Core/Templates/LuminaTemplate.h"
#include "Paths/Paths.h"


namespace Lumina::FileSystem
{
    static THashMap<FName, TVector<FFileSystem>> FileSystemStorage;
    
    
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
        FFixedString NormalizedPath = Paths::Normalize(Path);
        
        size_t LastSlash = Path.find_last_of('/');
        if (LastSlash == FString::npos)
        {
            return {};
        }
        
        FFixedString FilePart = NormalizedPath.left(LastSlash + 1);
        if (bRemoveExtension)
        {
            size_t DotPos = FilePart.find_last_of('.');
            if (DotPos != FString::npos)
            {
                return FilePart.left(DotPos);
            }
        }

        return FilePart;
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
        return std::filesystem::create_directory(Path.data());
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

    FStringView Parent(FStringView Path)
    {
        FFixedString Str(Path.begin(), Path.end());
        Paths::Normalize(Str);
        size_t Pos = Path.find_last_of('/');
        if (Pos == FString::npos)
        {
            return {};
        }
        
        return Path.substr(Pos + 1);
    }

    bool ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        return true;//return ResolvedPath.FileSystem->ReadFile(Result, Path);
    }

    bool ReadFile(FString& OutString, FStringView Path)
    {
        return true;//return ResolvedPath.FileSystem->ReadFile(OutString, Path);
    }

    bool WriteFile(FStringView Path, FStringView Data)
    {
        return true;//return ResolvedPath.FileSystem->WriteFile(Path, Data);
    }

    bool WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        return true;//return ResolvedPath.FileSystem->WriteFile(Path, Data);
    }
    
    FFileSystem& Detail::AddFileSystemImpl(const FName& Alias, FFileSystem&& System)
    {
        return FileSystemStorage[Alias].emplace_back(Move(System));
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

    void DirectoryIterator(FStringView Path, const TFunctionRef<void(FStringView Path)>& Func)
    {
        for (const std::filesystem::directory_entry& Directory : std::filesystem::directory_iterator(Path.data()))
        {
            Func(FStringView(Directory.path().generic_string().data(), Directory.path().generic_string().size()));
        }
    }

    void ForEachFileSystem(const TFunctionRef<void(FFileSystem&)>& Func)
    {
        for (auto& [Alias, FileSystems] : FileSystemStorage)
        {
            for (FFileSystem& System : FileSystems)
            {
                Func(System);
            }
        }
    }
}
