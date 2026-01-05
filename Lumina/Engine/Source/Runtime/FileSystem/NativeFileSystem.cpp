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

    bool FNativeFileSystem::ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        FFixedString NativeFilePath;
        if (Path.starts_with(AliasPath))
        {
            FStringView Remainder = Path.substr(AliasPath.length());
            NativeFilePath.append(BasePath.c_str()).append("/").append(Remainder.data());
        }
        
        Result.clear();

        std::ifstream File(NativeFilePath.data(), std::ios::binary | std::ios::ate);
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
        std::ifstream File(Path.data(), std::ios::binary);
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
        std::ofstream File(Path.data(), std::ios::binary | std::ios::trunc);
        if (!File)
        {
            return false;
        }

        File.write(Data.data(), static_cast<std::streamsize>(Data.size()));
        return File.good();
    }

    bool FNativeFileSystem::WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        std::ofstream OutFile(Path.data(), std::ios::binary | std::ios::trunc);
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
}
