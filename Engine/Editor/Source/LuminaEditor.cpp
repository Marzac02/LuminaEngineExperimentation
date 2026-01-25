#include "LuminaEditor.h"
#include <Assets/AssetRegistry/AssetRegistry.h>
#include <Containers/String.h>
#include <Core/Application/Application.h>
#include <Core/Engine/Engine.h>
#include <Core/Templates/Optional.h>
#include <FileSystem/NativeFileSystem.h>
#include <Log/Log.h>
#include <Memory/Memory.h>
#include <Tools/UI/DevelopmentToolUI.h>
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Scripting/Lua/Scripting.h"
#include "UI/EditorUI.h"

namespace Lumina
{
    EDITOR_API FEditorEngine* GEditorEngine = nullptr;
    
    bool FEditorEngine::Init()
    {
        TOptional<FFixedString> MaybeProject = FApplication::CommandLine.Get("project");
        
        if (MaybeProject.has_value())
        {
            LoadProject(MaybeProject.value());
        }
        else
        {
            LOG_INFO("Found no project in the command-line.");
        }
        
        bool bSuccess = FEngine::Init();
        
        if (!ProjectPath.empty())
        {
            FAssetRegistry::Get().RunInitialDiscovery();
        }

        entt::locator<entt::meta_ctx>::reset(GetEngineMetaService());

        return bSuccess;
    }

    bool FEditorEngine::Shutdown()
    {
        return FEngine::Shutdown();
    }
    
    IDevelopmentToolUI* FEditorEngine::CreateDevelopmentTools()
    {
        return Memory::New<FEditorUI>();
    }
    
    void FEditorEngine::LoadProject(FStringView Path)
    {
        namespace FS = FileSystem;
        
        ProjectPath                     .assign_convert(FS::Parent(Paths::Normalize(Path)));
        ProjectName                     = FS::FileName(Path, true);
        FFixedString GameDir            = Paths::Combine(ProjectPath, "Game");
        FFixedString BinariesDirectory  = Paths::Combine(ProjectPath, "Binaries");
        
        FS::Mount<FS::FNativeFileSystem>("/Game", GameDir);

        LoadProjectScripts();
        
        bHasLoadedProject = true;
        
        OnProjectLoaded.Broadcast();
    }

    FFixedString FEditorEngine::GetProjectScriptDirectory() const
    {
        if (!bHasLoadedProject)
        {
            return {};
        }
        
        return Paths::Combine(ProjectPath, "Game", "Scripts");
    }

    FFixedString FEditorEngine::GetProjectGameDirectory() const
    {
        if (!bHasLoadedProject)
        {
            return {};
        }
        
        return Paths::Combine(ProjectPath, "Game");

    }

    FFixedString FEditorEngine::GetProjectContentDirectory() const
    {
        if (!bHasLoadedProject)
        {
            return {};
        }
        
        return Paths::Combine(ProjectPath, "Game", "Content");
    }
    
    void FEditorEngine::LoadProjectScripts()
    {
        FFixedString ScriptPath;
        ScriptPath.append(ProjectPath).append("/Game/Scripts/");
        Scripting::FScriptingContext::Get().LoadScriptsInDirectoryRecursively(ScriptPath);
    }
}
