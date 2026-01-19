#pragma once
#include "Core/Delegates/Delegate.h"
#include "Renderer/BindingCache.h"
#include "Renderer/TypedBuffer.h"
#include "Renderer/Vertex.h"
#include "World/Scene/RenderScene/MeshDrawCommand.h"
#include "World/Scene/RenderScene/RenderScene.h"


namespace Lumina
{
    class CWorld;

    /**
     * Scene rendering via Clustered Forward Rendering.
     */
    class FForwardRenderScene : public IRenderScene
    {
    
    public:
        FForwardRenderScene(CWorld* InWorld);
        
        void Init() override;
        void Shutdown() override;
        void RenderScene(FRenderGraph& RenderGraph, const FViewVolume& ViewVolume) override;
        void SetViewVolume(const FViewVolume& ViewVolume) override;
        void SwapchainResized(glm::vec2 NewSize);
        
        void CompileDrawCommands(FRenderGraph& RenderGraph) override;
                

        //~ Begin Render Passes
        void ResetPass(FRenderGraph& RenderGraph);
        void CullPass(FRenderGraph& RenderGraph);
        void DepthPrePass(FRenderGraph& RenderGraph);
        void DepthPyramidPass(FRenderGraph& RenderGraph);
        void ClusterBuildPass(FRenderGraph& RenderGraph);
        void LightCullPass(FRenderGraph& RenderGraph);
        void PointShadowPass(FRenderGraph& RenderGraph);
        void SpotShadowPass(FRenderGraph& RenderGraph);
        void CascadedShowPass(FRenderGraph& RenderGraph);
        void BasePass(FRenderGraph& RenderGraph);
        void TransparentPass(FRenderGraph& RenderGraph);
        void EnvironmentPass(FRenderGraph& RenderGraph);
        void BatchedLineDraw(FRenderGraph& RenderGraph);
        void ToneMappingPass(FRenderGraph& RenderGraph);
        void DebugDrawPass(FRenderGraph& RenderGraph);
        //~ End Render Passes

        void InitBuffers();
        void InitImages();
        void InitFrameResources();

        static FViewportState MakeViewportStateFromImage(const FRHIImage* Image);

        void CheckInstanceBufferResize(uint32 NumInstances);
        void CheckLightBufferResize(uint32 NumLights);
        void CheckSimpleVertexResize(uint32 NumVertices);
        
        FRHIImage* GetRenderTarget() const override;
        FSceneRenderSettings& GetSceneRenderSettings() override;
        entt::entity GetEntityAtPixel(uint32 X, uint32 Y) const override;


        FDelegateHandle                     SwapchainResizedHandle;
        CWorld*                             World = nullptr;
        
        FSceneRenderSettings                RenderSettings;
        FSceneLightData                     LightData;

        /** Packed array of all light shadows in the scene */
        TArray<TVector<FLightShadow>, (uint32)ELightType::Num>    PackedShadows;

        FBindingCache                       BindingCache;

        FRHIViewportRef                     SceneViewport;
        
        FRHIInputLayoutRef                  SimpleVertexLayoutInput;

        FSceneGlobalData                    SceneGlobalData;

        FRHIBindingSetRef                   BasePassSet;
        FRHIBindingLayoutRef                BasePassLayout;

        FRHIBindingSetRef                   ToneMappingPassSet;
        FRHIBindingLayoutRef                ToneMappingPassLayout;
        
        FRHIBindingSetRef                   ShadowPassSet;
        FRHIBindingLayoutRef                ShadowPassLayout;

        FRHIBindingSetRef                   DebugPassSet;
        FRHIBindingLayoutRef                DebugPassLayout;

        FRHIBindingSetRef                   SSAOBlurPassSet;
        FRHIBindingLayoutRef                SSAOBlurPassLayout;
        
        FRHIBindingSetRef                   BindingSet;
        FRHIBindingLayoutRef                BindingLayout;

        FRHIBindingSetRef                   MeshCullSet;
        FRHIBindingLayoutRef                MeshCullLayout;

        FRHIBindingSetRef                   ClusterBuildSet;
        FRHIBindingLayoutRef                ClusterBuildLayout;

        FRHIBindingSetRef                   LightCullSet;
        FRHIBindingLayoutRef                LightCullLayout;

        FRHITypedVertexBuffer<FSimpleElementVertex> SimpleVertexBuffer;
        TVector<FSimpleElementVertex>               SimpleVertices;
        FRHIBindingLayoutRef                        SimplePassLayout;

        FRHIBufferRef                               CullDataBuffer;
        FRHIBufferRef                               ClusterBuffer;
        FRHIBufferRef                               SceneDataBuffer;
        FRHIBufferRef                               InstanceDataBuffer;
        FRHIBufferRef                               BoneDataBuffer;
        FRHIBufferRef                               InstanceMappingBuffer;
        FRHIBufferRef                               LightDataBuffer;
        FRHIBufferRef                               IndirectDrawBuffer;
        
        FShadowAtlas                        ShadowAtlas;

        FRHIImageRef                        HDRRenderTarget;
        FRHIImageRef                        CascadedShadowMap;
        FRHIImageRef                        DepthAttachment;
        FRHIImageRef                        DepthPyramid;
        FRHIImageRef                        PickerImage;
        
        /** Packed array of per-instance data */
        TVector<FInstanceData>                  InstanceData;
        TVector<glm::mat4>                      BonesData;
        
        
        FMeshPass DepthMeshPass;
        FMeshPass OpaqueMeshPass;
        FMeshPass TranslucentMeshPass;
        FMeshPass ShadowMeshPass;
        
        /** Packed array of all cached mesh draw commands */
        TVector<FMeshDrawCommand>             DrawCommands;

        /** Packed indirect draw arguments, gets sent directly to the GPU */
        TVector<FDrawIndexedIndirectArguments>  IndirectDrawArguments;
    };
}
