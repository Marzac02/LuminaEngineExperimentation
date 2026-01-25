#pragma once
#include "SceneRenderTypes.h"
#include "Platform/GenericPlatform.h"
#include "Renderer/RHIFwd.h"
#include "World/Scene/SceneInterface.h"


namespace Lumina
{
    class FRenderGraph;
    class FViewVolume;

    class IRenderScene : public ISceneInterface
    {
    public:

        virtual ~IRenderScene() = default;
        
        RUNTIME_API virtual void RenderScene(FRenderGraph& RenderGraph, const FViewVolume& ViewVolume) = 0;
        RUNTIME_API virtual void SetViewVolume(const FViewVolume& ViewVolume) = 0;
        RUNTIME_API virtual void CompileDrawCommands(FRenderGraph& RenderGraph) = 0;
        RUNTIME_API virtual FRHIImage* GetRenderTarget() const = 0;
        RUNTIME_API virtual FSceneRenderSettings& GetSceneRenderSettings() = 0;

        RUNTIME_API virtual entt::entity GetEntityAtPixel(uint32 X, uint32 Y) const = 0;
        
    };
}
