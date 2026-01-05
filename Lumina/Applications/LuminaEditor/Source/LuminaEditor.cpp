#include "LuminaEditor.h"
#include "EntryPoint.h"
#include "Core/Module/ModuleManager.h"
#include "Core/Windows/Window.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Renderer/RenderResource.h"
#include "UI/EditorUI.h"

namespace Lumina
{
    bool FEditorEngine::Init(FApplication* App)
    {
        InitializeCObjectSystem();
        
        TOptional<FString> MaybeProject = FApplication::CommandLine.Get("project");
        
        if (MaybeProject.has_value())
        {
            LoadProject(MaybeProject.value());
        }
        else
        {
            LOG_INFO("No project passed in via command-line");
        }
        
        bool bSuccess = FEngine::Init(App);
        
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
        FStringView Parent = FileSystem::Parent(Path);
        FFixedString GameDir = Paths::Combine(Parent, "Game");
        FFixedString ConfigDir = Paths::Combine(Parent, "Config");

        FileSystem::Mount<FileSystem::FNativeFileSystem>("/Game", GameDir);
        FileSystem::Mount<FileSystem::FNativeFileSystem>("/Config", ConfigDir);

    }

    FApplication* CreateApplication(int argc, char** argv)
    {
        return Memory::New<LuminaEditor>();
    }
    
    LuminaEditor::LuminaEditor()
        : FApplication("Lumina Editor", 1 << 0)
    {
    }
    
    bool LuminaEditor::Initialize(int argc, char** argv)
    {
        GEngine->Init(this);
        
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

    void LuminaEditor::RenderDeveloperTools(const FUpdateContext& UpdateContext)
    {
    }

    void LuminaEditor::Shutdown()
    {
        
    }
}


DECLARE_MODULE_ALLOCATOR_OVERRIDES()

