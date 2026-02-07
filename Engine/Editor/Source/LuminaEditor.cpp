#include "LuminaEditor.h"
#include <Core/Engine/Engine.h>
#include <Memory/Memory.h>
#include <Tools/UI/DevelopmentToolUI.h>

#include "Config/Config.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "UI/EditorUI.h"
#include "World/WorldManager.h"

namespace Lumina
{
    EDITOR_API FEditorEngine* GEditorEngine = nullptr;
    
    bool FEditorEngine::Init()
    {
        VFS::Mount<VFS::FNativeFileSystem>("/Editor", Paths::Combine(Paths::GetEngineDirectory(), "Editor"));
        
        GConfig->LoadPath("/Editor/Config");
        
        bool bSuccess = FEngine::Init();
        
        entt::locator<entt::meta_ctx>::reset(GetEngineMetaService());

        return bSuccess;
    }

    bool FEditorEngine::Shutdown()
    {
        return FEngine::Shutdown();
    }

    CWorld* FEditorEngine::GetCurrentEditorWorld() const
    {
        return nullptr;
    }

    IDevelopmentToolUI* FEditorEngine::CreateDevelopmentTools()
    {
        return Memory::New<FEditorUI>();
    }
}
