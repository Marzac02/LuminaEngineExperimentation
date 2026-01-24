#include "LuminaEditor.h"
#include "Core/Application/ApplicationGlobalState.h"
#include "Core/Module/ModuleManager.h"
#include "Core/Windows/Window.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Scripting/Lua/Scripting.h"
#include "UI/EditorUI.h"
#ifdef LE_PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include <cstdlib>
#include <Assets/AssetRegistry/AssetRegistry.h>
#include <Containers/String.h>
#include <Core/Application/Application.h>
#include <Core/Engine/Engine.h>
#include <Core/Module/ModuleInterface.h>
#include <Core/Object/ObjectBase.h>
#include <Core/Templates/Optional.h>
#include <Core/UpdateContext.h>
#include <Core/Windows/WindowTypes.h>
#include <FileSystem/NativeFileSystem.h>
#include <Log/Log.h>
#include <Memory/Memory.h>
#include <Tools/UI/DevelopmentToolUI.h>
#include <entt/entt.hpp>
#include <spdlog/details/windows_include.h>

namespace Lumina
{
    FEditorEngine* GEditorEngine = nullptr;
    
    bool FEditorEngine::Init()
    {
        InitializeCObjectSystem();
        
        TOptional<FFixedString> MaybeProject = FApplication::CommandLine.Get("project");
        
        if (MaybeProject.has_value())
        {
            LoadProject(MaybeProject.value());
        }
        else
        {
            LOG_INFO("No project passed in via command-line");
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

        LoadProjectDLL();
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

    void FEditorEngine::LoadProjectDLL()
    {
        
        FFixedString DLLPath;
        
        #if LE_DEBUG
        DLLPath.append(ProjectPath).append("/Binaries/Debug/").append_convert(ProjectName).append(".dll");
        #else
        DLLPath.append(ProjectPath).append("/Binaries/Release/").append_convert(ProjectName).append(".dll");
        #endif

        if (Paths::Exists(DLLPath))
        {
            if (IModuleInterface* Module = FModuleManager::Get().LoadModule(DLLPath))
            {
                ProcessNewlyLoadedCObjects();
            }
            else
            {
                LOG_INFO("No project module found");
            }
        }
    }

    void FEditorEngine::LoadProjectScripts()
    {
        FFixedString ScriptPath;
        ScriptPath.append(ProjectPath).append("/Game/Scripts/");
        Scripting::FScriptingContext::Get().LoadScriptsInDirectoryRecursively(ScriptPath);
    }
    
    LuminaEditor::LuminaEditor()
        : FApplication("Lumina Editor", 1 << 0)
    {
    }
    
    bool LuminaEditor::Initialize(int argc, char** argv)
    {
        GEngine->Init();
        
        return true;
    }

    FEngine* LuminaEditor::CreateEngine()
    {
        GEditorEngine = Memory::New<FEditorEngine>();
        GEngine = GEditorEngine;
        return GEngine;
    }

    bool LuminaEditor::ShouldExit() const
    {
        return MainWindow->ShouldClose() || bExitRequested;
    }

    void LuminaEditor::CreateProject()
    {
        
    }

    void LuminaEditor::OpenProject()
    {
        
    }

    FWindowSpecs LuminaEditor::GetWindowSpecs() const
    {
        FWindowSpecs AppWindowSpecs;
        AppWindowSpecs.Title = ApplicationName.c_str();
        return AppWindowSpecs;
    }

    void LuminaEditor::RenderDeveloperTools(const FUpdateContext& UpdateContext)
    {
    }

    void LuminaEditor::Shutdown()
    {
        
    }
}


static int GuardedMain(int argc, char** argv)
{
    int Result = 0;
    {
        Lumina::FApplicationGlobalState GlobalState;
        Lumina::FApplication* App = Lumina::Memory::New<Lumina::LuminaEditor>();
        Result = App->Run(argc, argv);
    }
	
    return Result;
}

#if LE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    return GuardedMain(__argc, __argv);
}
#endif

int main(int argc, char** argv)
{
    return GuardedMain(argc, argv);
}


DECLARE_MODULE_ALLOCATOR_OVERRIDES()

