#pragma once

#include "Core/UpdateContext.h"
#include "Subsystems/Subsystem.h"
#include <entt/entt.hpp>


namespace Lumina
{
    class FRHIViewport;
    class FWorldManager;
    class FAssetRegistry;
    class FRenderManager;
    class IImGuiRenderer;
    class IDevelopmentToolUI;
    class FAssetManager;
    class FApplication;
    class FWindow;
}

namespace Lumina
{
    class FEngine
    {
    public:
        
        FEngine() = default;
        virtual ~FEngine() = default;

        RUNTIME_API virtual bool Init();
        RUNTIME_API virtual bool Shutdown();
        RUNTIME_API bool Update(bool bApplicationWantsExit);
        RUNTIME_API virtual void OnUpdateStage(const FUpdateContext& Context) { }

        RUNTIME_API static FRHIViewport* GetEngineViewport();
        
        RUNTIME_API void SetEngineViewportSize(const glm::uvec2& InSize);

        #if WITH_EDITOR
        RUNTIME_API virtual IDevelopmentToolUI* CreateDevelopmentTools() = 0;
        RUNTIME_API virtual void DrawDevelopmentTools();
        RUNTIME_API IDevelopmentToolUI* GetDevelopmentToolsUI() const { return DeveloperToolUI; }
        #endif

        RUNTIME_API entt::meta_ctx& GetEngineMetaContext() const;
        RUNTIME_API entt::locator<entt::meta_ctx>::node_type GetEngineMetaService() const;

        RUNTIME_API void SetReadyToClose(bool bReadyToClose) { bEngineReadyToClose = bReadyToClose; }
        
        RUNTIME_API NODISCARD double GetDeltaTime() const { return UpdateContext.DeltaTime; }
        
        template<typename T>
        requires(eastl::is_base_of_v<ISubsystem, T>)
        T* GetEngineSubsystem()
        {
            return EngineSubsystems.GetSubsystem<T>();
        }

        FORCEINLINE const FUpdateContext& GetUpdateContext() const { return UpdateContext; }

        RUNTIME_API void SetEngineReadyToClose(bool bReady) { bEngineReadyToClose = bReady; }
        RUNTIME_API bool IsCloseRequested() const { return bCloseRequested; }
    
    protected:
        
        FUpdateContext          UpdateContext;

        #if WITH_EDITOR
        IDevelopmentToolUI*     DeveloperToolUI =       nullptr;
        #endif

        FSubsystemManager       EngineSubsystems;
        FWorldManager*          WorldManager =          nullptr;
        FRenderManager*         RenderManager =         nullptr;
        

        bool                    bCloseRequested = false;
        bool                    bEngineReadyToClose = false;
    };
    
    RUNTIME_API extern FEngine* GEngine;
    
    template<typename T>
    requires(eastl::is_base_of_v<ISubsystem, T>)
    static T& GetEngineSystem()
    {
        return *GEngine->GetEngineSubsystem<T>();
    }
    
}
