#include "pch.h"
#include "Project.h"
#include "Log/Log.h"
#include "Platform/Filesystem/FileHelper.h"
#include <string>
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Module/ModuleManager.h"
#include "Core/Object/ObjectBase.h"
#include "Paths/Paths.h"
#include "Scripting/Lua/Scripting.h"


namespace Lumina
{
    FProject::FProject()
        : bHasProjectLoaded(false)
    {
    }

    void FProject::LoadProject(const FString& ProjectPath)
    {
        if(!Paths::Exists(ProjectPath))
        {
            LOG_WARN("Invalid project path given: {}", ProjectPath);
            return;
        }

        Settings.ProjectName = Paths::FileName(ProjectPath, true);
        Settings.ProjectPath = ProjectPath;
        
        Paths::Mount("project://", GetProjectGameDirectory());

        Scripting::FScriptingContext::Get().LoadScriptsInDirectoryRecursively(GetProjectScriptsDirectory());
        
        FString ProjectSolutionPath = Paths::Parent(Paths::RemoveExtension(ProjectPath));
#if LE_DEBUG
		FString Path = ProjectSolutionPath + "/Binaries/Debug/" + Settings.ProjectName + ".dll";
#else
        FString Path = ProjectSolutionPath + "/Binaries/Release/" + Settings.ProjectName + ".dll";
#endif

        if (Paths::Exists(Path))
        {
            if (IModuleInterface* Module = FModuleManager::Get().LoadModule(Path))
            {
                ProcessNewlyLoadedCObjects();
            }
            else
            {
                LOG_INFO("No project module found");
            }
        }

        LOG_INFO("Loaded Project: {}", ProjectPath);
        bHasProjectLoaded = true;
        OnProjectLoaded.Broadcast();
    }

    FString FProject::GetProjectRootDirectory() const
    {
        std::filesystem::path Path = GetProjectSettings().ProjectPath.c_str();
        Path = Path.parent_path();

        return FString(Path.string().c_str());
    }

    FString FProject::GetProjectContentDirectory() const
    {
        std::filesystem::path Path = GetProjectSettings().ProjectPath.c_str();
        Path = Path.parent_path() / "Game" / "Content";
        FString StringPath(Path.string().c_str());

        Paths::NormalizePath(StringPath);
        
        return StringPath;
    }

    FString FProject::GetProjectGameDirectory() const
    {
        std::filesystem::path Path = GetProjectSettings().ProjectPath.c_str();
        Path = Path.parent_path() / "Game";
        FString StringPath(Path.string().c_str());

        Paths::NormalizePath(StringPath);
        return StringPath;
    }

    FString FProject::GetProjectScriptsDirectory() const
    {
        std::filesystem::path Path = GetProjectSettings().ProjectPath.c_str();
        Path = Path.parent_path() / "Game" / "Scripts";
        FString StringPath(Path.string().c_str());

        Paths::NormalizePath(StringPath);
        
        return StringPath;
    }

    FString FProject::GetProjectContentBinDirectory() const
    {
        return GetProjectContentDirectory() + "/Bin";
    }
}
