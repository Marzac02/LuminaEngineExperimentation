#pragma once

#include "Core/Templates/LuminaTemplate.h"
#include "Containers/Name.h"
#include "Log/Log.h"

namespace Lumina::Paths
{
    void InitializePaths();
    
    
    /** Gets the directory where the Lumina engine is installed. */
    LUMINA_API FString GetEngineDirectory();

    LUMINA_API FString GetExtension(const FString& InPath);
    
    LUMINA_API bool IsDirectory(const FString& Path);
    
    /** Removes and returns the path without its file extension. */
    LUMINA_API FString RemoveExtension(const FString& InPath);
    
    /** Checks whether the given file or directory exists. */
    LUMINA_API bool Exists(FStringView Filename);
    
    /** Returns the virtual path prefix from a virtual path (Assuming the path given is already a virtual path) */
    LUMINA_API FString GetVirtualPathPrefix(const FString& VirtualPath);
    
    LUMINA_API bool CreateDirectories(FStringView Path);
    
    /** 
     * Checks if the given directory is under a specific parent directory.
     * This does a path-based comparison, not a file system query.
     */
    LUMINA_API bool IsUnderDirectory(const FString& ParentDirectory, const FString& Directory);

    /**
     * Makes a path relative to a base path.
     * For example, MakeRelativeTo("/a/b/c.txt", "/a/") would return "b/c.txt".
     */
    LUMINA_API FString MakeRelativeTo(const FString& Path, const FString& BasePath);
    
    
    /**
     * Replaces the filename portion of a given path.
     * @param Path The full path whose filename will be replaced.
     * @param NewFilename The new filename to insert.
     */
    LUMINA_API void ReplaceFilename(FString& Path, const FString& NewFilename);
    
    /** Gets the path to the engine's resource directory. */
    LUMINA_API const FString& GetEngineResourceDirectory();
    
    /** Gets the path to the engine's font directory. */
    LUMINA_API const FString& GetEngineFontDirectory();

    /** Gets the path to the engine's content directory */
    LUMINA_API const FString& GetEngineContentDirectory();
    
    
    LUMINA_API const FString& GetEngineConfigDirectory();

    /** Gets the path to the engine's shaders */
    LUMINA_API const FString& GetEngineShadersDirectory();
    
    /** Gets the engine installation directory (one level above the engine binary). */
    LUMINA_API FString GetEngineInstallDirectory();

    LUMINA_API void Normalize(FString& Path);
    LUMINA_API void Normalize(FFixedString& Path);
    LUMINA_API FFixedString Normalize(FStringView Path);

    LUMINA_API bool PathsEqual(FStringView A, FStringView B);

    LUMINA_API FString Parent(FStringView Path, bool bRemoveTrailingSlash = true);

    
    /**
     * Sets an environment variable in a cross-platform way.
     * @param name The name of the environment variable.
     * @param value The value to set.
     * @return True if the operation succeeded, false otherwise.
     */
    LUMINA_API bool SetEnvVariable(const FString& name, const FString& value);
    
    
    // -------------------------------------------------------------------

    template<typename T>
    concept ValidStringType = requires(T s)
    {
        typename T::value_type;
    } && (
        requires(T s) { { s.c_str() } -> std::convertible_to<const T::value_type*>; } ||
        requires(T s) { { s.data() } -> std::convertible_to<const T::value_type*>; }
    );
    
    template <typename... Paths>
    NODISCARD FFixedString Combine(Paths&&... InPaths)
    {
        FFixedString Result;

        auto AppendPath = [&Result, bFirst = true](FStringView Path) mutable
        {
            if (Path.empty())
            {
                return;
            }
    
            if (!bFirst && !Result.empty() && Result.back() != '/')
            {
                Result += '/';
            }
    
            if (!bFirst && !Path.empty() && Path.front() == '/')
            {
                Path = Path.substr(1);
            }
        
            Result.append(Path.data(), Path.length());
            bFirst = false;
        };

        (AppendPath(Forward<Paths>(InPaths)), ...);

        return Result;
    }
    

    template<ValidStringType StringType>
    NODISCARD StringType DirName(const StringType& String)
    {
        size_t LastSlash = String.find_last_of("/\\");
        if (LastSlash != FString::npos)
        {
            return String.substr(0, LastSlash);
        }
        return String;
    }
}
