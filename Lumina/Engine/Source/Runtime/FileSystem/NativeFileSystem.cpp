#include "pch.h"
#include "NativeFileSystem.h"
#include <fstream>


namespace Lumina::FileSystem
{
    FNativeFileSystem::FNativeFileSystem(const FName& InAliasPath, FStringView InBasePath) noexcept
        : AliasPath(InAliasPath.c_str())
        , BasePath(InBasePath.begin(), InBasePath.end())
    {
    }

    FFixedString FNativeFileSystem::ResolveVirtualPath(FStringView Path) const
    {
        if (!Path.starts_with(AliasPath))
        {
            return {};
        }
    
        FStringView RelativePath = Path.substr(AliasPath.length());
    
        FFixedString FullPath = BasePath;
        FullPath.append(RelativePath.begin(), RelativePath.end());
        
        return FullPath;
    }

    bool FNativeFileSystem::ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        FFixedString FullPath = ResolveVirtualPath(Path);
        
        Result.clear();

        std::ifstream File(FullPath.data(), std::ios::binary | std::ios::ate);
        if (!File)
        {
            return false;
        }

        const std::streamsize Size = File.tellg();
        if (Size < 0)
        {
            return false;
        }

        if (Size == 0)
        {
            return true;
        }

        File.seekg(0, std::ios::beg);

        Result.resize(static_cast<size_t>(Size));

        if (!File.read(reinterpret_cast<char*>(Result.data()), Size))
        {
            Result.clear();
            return false;
        }

        return true;
    }


    bool FNativeFileSystem::ReadFile(FString& OutString, FStringView Path)
    {
        FFixedString FullPath = ResolveVirtualPath(Path);

        std::ifstream File(FullPath.data(), std::ios::binary);
        if (!File)
        {
            return false;
        }

        File.seekg(0, std::ios::end);
        std::streamsize Size = File.tellg();
        File.seekg(0, std::ios::beg);

        if (Size < 0)
        {
            return false;
        }

        OutString.resize(static_cast<size_t>(Size));

        if (!File.read(OutString.data(), Size))
        {
            return false;
        }

        return true;
    }

    bool FNativeFileSystem::WriteFile(FStringView Path, FStringView Data)
    {
        FFixedString FullPath = ResolveVirtualPath(Path);
        std::ofstream File(FullPath.data(), std::ios::binary | std::ios::trunc);
        if (!File)
        {
            return false;
        }

        File.write(Data.data(), static_cast<std::streamsize>(Data.size()));
        return File.good();
    }

    bool FNativeFileSystem::WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        FFixedString FullPath = ResolveVirtualPath(Path);
        std::ofstream OutFile(FullPath.data(), std::ios::binary | std::ios::trunc);
        if (!OutFile)
        {
            return false;
        }

        if (!OutFile.write(reinterpret_cast<const char*>(Data.data()), static_cast<int64>(Data.size())))
        {
            return false;
        }

        return true;
    }

    bool FNativeFileSystem::Exists(FStringView Path) const
    {
        return std::filesystem::exists(ResolveVirtualPath(Path).c_str());
    }

    bool FNativeFileSystem::CreateDir(FStringView Path) const
    {
        return std::filesystem::create_directory(ResolveVirtualPath(Path).c_str());
    }

    bool FNativeFileSystem::Remove(FStringView Path) const
    {
        return std::filesystem::remove(ResolveVirtualPath(Path).c_str());
    }

    bool FNativeFileSystem::RemoveAll(FStringView Path) const
    {
        return std::filesystem::remove_all(ResolveVirtualPath(Path).c_str());
    }

    bool FNativeFileSystem::Rename(FStringView Old, FStringView New) const
    {
        std::filesystem::rename(ResolveVirtualPath(Old).c_str(), ResolveVirtualPath(New).c_str());
        return true;
    }
}
