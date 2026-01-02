#include "pch.h"
#include "ForwardRenderScene.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "Core/Console/ConsoleVariable.h"
#include "Core/Windows/Window.h"
#include "Memory/Memcpy.h"
#include "Renderer/RendererUtils.h"
#include "Renderer/RHIStaticStates.h"
#include "Renderer/ShaderCompiler.h"
#include "Renderer/TypedBuffer.h"
#include "Renderer/RenderGraph/RenderGraphDescriptor.h"
#include "World/World.h"
#include "world/entity/components/environmentcomponent.h"
#include "world/entity/components/lightcomponent.h"
#include "World/Entity/Components/LineBatcherComponent.h"
#include "world/entity/components/staticmeshcomponent.h"
#include "World/Scene/RenderScene/MeshDrawCommand.h"

namespace Lumina
{
    static TConsoleVar CVarShadowCascadeLambda("Rendering.Shadows.CascadeLambda", 0.95f, "Changes the lambda of the cascade selection");
    static TConsoleVar CVarRenderBasePass("Rendering.Passes.BasePass", true, "Toggles rendering of the base pass");

    FForwardRenderScene::FForwardRenderScene(CWorld* InWorld)
        : World(InWorld)
        , LightData(), SceneGlobalData()
        , ShadowAtlas(FShadowAtlasConfig())
        , DebugVisualizationMode()
        , ShadowCascades()
        , DepthMeshPass()
        , OpaqueMeshPass()
        , TranslucentMeshPass()
        , ShadowMeshPass()
    {
    }

    void FForwardRenderScene::Init()
    {
        LOG_TRACE("Initializing Forward Render Scene");
        
        DebugVisualizationMode = ERenderSceneDebugFlags::None;
        SceneViewport = GRenderContext->CreateViewport(Windowing::GetPrimaryWindowHandle()->GetExtent(), "Forward Renderer Viewport");

        InitBuffers();
        InitFrameResources();

        SwapchainResizedHandle = FRenderManager::OnSwapchainResized.AddMember(this, &FForwardRenderScene::SwapchainResized); 

        Memory::Memzero(&LightData, sizeof(FSceneLightData));
        
    }

    void FForwardRenderScene::Shutdown()
    {
        GRenderContext->WaitIdle();
        GRenderContext->ClearCommandListCache();
        GRenderContext->ClearBindingCaches();

        FRenderManager::OnSwapchainResized.Remove(SwapchainResizedHandle);
        
        LOG_TRACE("Shutting down Forward Render Scene");
    }

    void FForwardRenderScene::RenderScene(FRenderGraph& RenderGraph, const FViewVolume& ViewVolume)
    {
        LUMINA_PROFILE_SCOPE();
            
        SetViewVolume(ViewVolume);

        // Wait for shader tasks.
        if(GRenderContext->GetShaderCompiler()->HasPendingRequests())
        {
            return;
        }

        ResetPass(RenderGraph);
        CompileDrawCommands(RenderGraph);
        CullPass(RenderGraph);
        DepthPrePass(RenderGraph);
        DepthPyramidPass(RenderGraph);
        ClusterBuildPass(RenderGraph);
        LightCullPass(RenderGraph);
        PointShadowPass(RenderGraph);
        SpotShadowPass(RenderGraph);
        DirectionalShowPass(RenderGraph);
        EnvironmentPass(RenderGraph);
        BasePass(RenderGraph);
        TransparentPass(RenderGraph);
        BatchedLineDraw(RenderGraph);
        ToneMappingPass(RenderGraph);
        DebugDrawPass(RenderGraph);
    }

    void FForwardRenderScene::SetViewVolume(const FViewVolume& ViewVolume)
    {
        SceneViewport->SetViewVolume(ViewVolume);
        
        SceneGlobalData.CameraData.Location             = glm::vec4(SceneViewport->GetViewVolume().GetViewPosition(), 1.0f);
        SceneGlobalData.CameraData.View                 = SceneViewport->GetViewVolume().GetViewMatrix();
        SceneGlobalData.CameraData.InverseView          = SceneViewport->GetViewVolume().GetInverseViewMatrix();
        SceneGlobalData.CameraData.Projection           = SceneViewport->GetViewVolume().GetProjectionMatrix();
        SceneGlobalData.CameraData.InverseProjection    = SceneViewport->GetViewVolume().GetInverseProjectionMatrix();
        SceneGlobalData.ScreenSize                      = glm::vec4(SceneViewport->GetSize().x, SceneViewport->GetSize().y, 0.0f, 0.0f);
        SceneGlobalData.GridSize                        = glm::vec4(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ, 0.0f);
        SceneGlobalData.Time                            = (float)World->GetTimeSinceWorldCreation();
        SceneGlobalData.DeltaTime                       = (float)World->GetWorldDeltaTime();
        SceneGlobalData.FarPlane                        = SceneViewport->GetViewVolume().GetFar();
        SceneGlobalData.NearPlane                       = SceneViewport->GetViewVolume().GetNear();
    }

    void FForwardRenderScene::SwapchainResized(glm::vec2 NewSize)
    {
        SceneViewport = GRenderContext->CreateViewport(NewSize, "Forward Renderer Viewport");
        
        InitFrameResources();
        BindingCache.ReleaseResources();
    }

    void FForwardRenderScene::CompileDrawCommands(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();

        {
            LUMINA_PROFILE_SECTION("Compile Draw Commands");
            
            auto View = World->GetEntityRegistry().view<SStaticMeshComponent, STransformComponent>();
            
            const size_t EntityCount = View.size_hint();
            const size_t EstimatedProxies = EntityCount * 2;

            InstanceData.reserve(EstimatedProxies);
            IndirectDrawArguments.reserve(EstimatedProxies);
			DrawCommands.reserve(EstimatedProxies);
            
            TFixedHashMap<uint64, uint64, 4> BatchedDraws;
            
            //========================================================================================================================
            {
                View.each([&](entt::entity Entity, const SStaticMeshComponent& MeshComponent, const STransformComponent& TransformComponent)
                {
                    CStaticMesh* Mesh = MeshComponent.StaticMesh;
                    if (!IsValid(Mesh))
                    {
                        return;
                    }
        
                    const FMeshResource& Resource = Mesh->GetMeshResource();
                    const uintptr_t MeshPtr = reinterpret_cast<uintptr_t>(Mesh);
                    const uint64 MeshID = (MeshPtr & 0xFFFFFull) << 24;

                    glm::mat4 TransformMatrix = TransformComponent.GetMatrix();
                    
                    FAABB BoundingBox       = Mesh->GetAABB().ToWorld(TransformMatrix);
                    glm::vec3 Center        = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
                    glm::vec3 Extents       = BoundingBox.Max - Center;
                    float Radius            = glm::length(Extents);
                    glm::vec4 SphereBounds  = glm::vec4(Center, Radius);
                    
                    for (const FGeometrySurface& Surface : Resource.GeometrySurfaces)
                    {
                        CMaterialInterface* Material = MeshComponent.GetMaterialForSlot(Surface.MaterialIndex);
                    
                        if (!IsValid(Material) || !Material->IsReadyForRender())
                        {
                            Material = CMaterial::GetDefaultMaterial();
                        }
                    
                        const uintptr_t MaterialPtr = reinterpret_cast<uintptr_t>(Material);
                        const uint64 MaterialID = (MaterialPtr & 0xFFFFFFFull) << 28;
                        uint64 SortKey = MaterialID | MeshID;
                        
                        if (RenderSettings.bUseInstancing == false)
                        {
                            SortKey = entt::to_integral(Entity);
                        }
                        
                        if (BatchedDraws.find(SortKey) == BatchedDraws.end())
                        {
                            BatchedDraws[SortKey] = IndirectDrawArguments.size();
        
                            DrawCommands.emplace_back(FMeshDrawCommand
                            {
                                .Material = Material,
                                .IndexBuffer =  Mesh->GetIndexBuffer(),
                                .VertexBuffer = Mesh->GetVertexBuffer(),
                                .IndirectDrawOffset = (uint32)IndirectDrawArguments.size()
                            });
                        
                            IndirectDrawArguments.emplace_back(FDrawIndexedIndirectArguments
                            {
                                .IndexCount = Surface.IndexCount,
                                .InstanceCount = 1,
                                .StartIndexLocation = Surface.StartIndex,
                                .BaseVertexLocation = 0,
                                .StartInstanceLocation = 0,
                            });
                        }
                        else
                        {
                            IndirectDrawArguments[BatchedDraws[SortKey]].InstanceCount++;
                        }
                        
                        InstanceData.emplace_back(FInstanceData
                        {
                            .Transform      = TransformMatrix,
                            .SphereBounds   = SphereBounds,
                            .EntityID       = entt::to_integral(Entity),
                            .BatchedDrawID  = (uint32)BatchedDraws[SortKey],
                            .bSelected      = Entity == World->GetSelectedEntity(),
                            .Reserved       = 0,
                        });   
                    }
                });
            }
        
            // Give each indirect draw command the correct start instance offset index.
            uint32 CumulativeInstanceCount = 0;
            for (FDrawIndexedIndirectArguments& Arg : IndirectDrawArguments)
            {
                Arg.StartInstanceLocation = CumulativeInstanceCount;
                CumulativeInstanceCount += Arg.InstanceCount;
            }
        
            // Since this value will be written in the shader, we no longer need it, since we've generated unique StartInstanceIndex per draw.
            // It must be reset to 0 because the computer shader atomically increments it, assuming 0 as the start.
            for (FDrawIndexedIndirectArguments& DrawArgs : IndirectDrawArguments)
            {
                DrawArgs.InstanceCount = 0;
            }
        }
        
        
        //========================================================================================================================
        
        {
            auto View = World->GetEntityRegistry().view<FLineBatcherComponent>();
            View.each([&](FLineBatcherComponent& LineBatcherComponent)
            {
                if (LineBatcherComponent.Lines.empty())
                {
                    return;
                }
        
                for (FLineBatcherComponent::FLineInstance& Line : LineBatcherComponent.Lines)
                {
                    if (Line.RemainingLifetime >= 0.0f)
                    {
                        Line.RemainingLifetime -= SceneGlobalData.DeltaTime;
                    }
                }
        
                TVector<FLineBatcherComponent::FLineInstance> NewLines;
                TVector<FSimpleElementVertex> NewVertices;
                
                NewLines.reserve(LineBatcherComponent.Lines.size());
                NewVertices.reserve(LineBatcherComponent.Vertices.size());
        
                uint32 CurrentVertexIndex = 0;
                for (const FLineBatcherComponent::FLineInstance& Line : LineBatcherComponent.Lines)
                {
                    if (Line.RemainingLifetime > 0.0f)
                    {
                        FLineBatcherComponent::FLineInstance NewLine = Line;
                        NewLine.StartVertexIndex = CurrentVertexIndex;
                        NewLines.push_back(NewLine);
        
                        NewVertices.push_back(LineBatcherComponent.Vertices[Line.StartVertexIndex]);
                        NewVertices.push_back(LineBatcherComponent.Vertices[Line.StartVertexIndex + 1]);
                        
                        CurrentVertexIndex += 2;
                    }
                }
        
                LineBatcherComponent.Lines = std::move(NewLines);
                LineBatcherComponent.Vertices = std::move(NewVertices);
        
                if (!LineBatcherComponent.Vertices.empty())
                {
                    SimpleVertices.resize(LineBatcherComponent.Vertices.size());
                    Memory::Memcpy(SimpleVertices.data(), 
                                  LineBatcherComponent.Vertices.data(), 
                                  LineBatcherComponent.Vertices.size() * sizeof(FSimpleElementVertex));
                }
            });
        }
        
        //========================================================================================================================
        
        {
            LightData.bHasSun = false;
            
            auto View = World->GetEntityRegistry().view<SDirectionalLightComponent>();
            View.each([this](const SDirectionalLightComponent& DirectionalLightComponent)
            {
                LightData.bHasSun = true;
        
                // Setup light data
                FLight Light;
                Light.Flags         = LIGHT_TYPE_DIRECTIONAL;
                Light.Color         = PackColor(glm::vec4(DirectionalLightComponent.Color, 1.0));
                Light.Intensity     = DirectionalLightComponent.Intensity;
                Light.Direction     = glm::normalize(DirectionalLightComponent.Direction);
        
                const FViewVolume& ViewVolume = SceneViewport->GetViewVolume();
        
                float NearClip = ViewVolume.GetNear();
                float FarClip = ViewVolume.GetFar();
                float ClipRange = FarClip - NearClip;
        
                float MinZ = NearClip;
                float MaxZ = NearClip + ClipRange;
        
                float Range = MaxZ - MinZ;
                float Ratio = MaxZ / MinZ;
        
                float CascadeSplits[NumCascades];
                for (uint32 i = 0; i < NumCascades; i++)
                {
                    float P = ((float)i + 1) / (float)NumCascades;
                    float Log = MinZ * glm::pow(Ratio, P);
                    float Uniform = MinZ + Range * P;
                    float D = CVarShadowCascadeLambda.GetValue() * (Log - Uniform) + Uniform;
                    CascadeSplits[i] = (D - NearClip) / ClipRange;
                }
        
                // For each cascade
                float LastSplitDist = 0.0;
                for (int i = 0; i < NumCascades; ++i)
                {
                    float SplitDist = CascadeSplits[i];
                    LightData.CascadeSplits[i] = SplitDist;
        
                    FShadowCascade& Cascade = ShadowCascades[i];
                    Cascade.DirectionalLight = Light;
        
                    glm::vec3 FrustumCorners[8];
                    FFrustum::ComputeFrustumCorners(ViewVolume.ToReverseDepthViewProjectionMatrix(), FrustumCorners);
                    
                    for (uint32 j = 0; j < 4; j++)
                    {
                        glm::vec3 dist = FrustumCorners[j + 4] - FrustumCorners[j];
                        FrustumCorners[j + 4] = FrustumCorners[j] + (dist * SplitDist);
                        FrustumCorners[j] = FrustumCorners[j] + (dist * LastSplitDist);
                    }
        
                    glm::vec3 FrustumCenter = glm::vec3(0.0f);
                    for (uint32 j = 0; j < 8; j++)
                    {
                        FrustumCenter += FrustumCorners[j];
                    }
                    FrustumCenter /= 8.0f;
        
        
                    float Radius = 0.0f;
                    for (uint32 j = 0; j < 8; j++)
                    {
                        float distance = glm::length(FrustumCorners[j] - FrustumCenter);
                        Radius = glm::max(Radius, distance);
                    }
                    Radius = glm::ceil(Radius * 16.0f) / 16.0f;
        
                    glm::vec3 MaxExtents = glm::vec3(Radius);
                    glm::vec3 MinExtents = -MaxExtents;
        
                    glm::vec3 LightDir = -Light.Direction;
                    glm::mat4 LightViewMatrix = glm::lookAt(FrustumCenter - LightDir * -MinExtents.z, FrustumCenter, FViewVolume::UpAxis);
        
                    glm::vec3 FrustumCenterLS = LightViewMatrix * glm::vec4(FrustumCenter, 1.0f);
                    float TexelSize = (Radius * 2.0f) / (float)GCSMResolution;
                    FrustumCenterLS.x = glm::floor(FrustumCenterLS.x / TexelSize) * TexelSize;
                    FrustumCenterLS.y = glm::floor(FrustumCenterLS.y / TexelSize) * TexelSize;
        
                    glm::mat4 InvLightViewMatrix = glm::inverse(LightViewMatrix);
                    glm::vec3 SnappedFrustumCenter = InvLightViewMatrix * glm::vec4(FrustumCenterLS, 1.0f);
        
                    LightViewMatrix = glm::lookAt(SnappedFrustumCenter - LightDir * -MinExtents.z, SnappedFrustumCenter, FViewVolume::UpAxis);
        
                    glm::mat4 LightOrthoMatrix = glm::ortho(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, 0.0f, MaxExtents.z - MinExtents.z);
        
                    Cascade.SplitDepth = (NearClip + SplitDist * ClipRange) * -1.0f;
                    Cascade.LightViewProjection = LightOrthoMatrix * LightViewMatrix;
        
                    Light.ViewProjection[i] = Cascade.LightViewProjection;
        
                    Cascade.ShadowMapSize = glm::uvec2(GCSMResolution);
        
                    LastSplitDist = CascadeSplits[i];
                }
        
                LightData.SunDirection = Light.Direction;
                LightData.NumLights++;
                
                LightData.Lights[0] = Light;
            });
        }
        
        //========================================================================================================================
        
        {
            auto View = World->GetEntityRegistry().view<SPointLightComponent, STransformComponent>();
            View.each([&] (SPointLightComponent& PointLightComponent, STransformComponent& TransformComponent)
            {
                FLight Light;
                Light.Flags                 = LIGHT_TYPE_POINT;
                Light.Falloff               = PointLightComponent.Falloff;
                Light.Color                 = PackColor(glm::vec4(PointLightComponent.LightColor, 1.0));
                Light.Intensity             = PointLightComponent.Intensity;
                Light.Radius                = PointLightComponent.Attenuation;
                Light.Position              = TransformComponent.WorldTransform.Location;
                
                FViewVolume LightView(90.0f, 1.0f, 0.01f, Light.Radius);
                
                auto SetView = [&Light](FViewVolume& View, uint32 Index)
                {
                    switch (Index)
                    {
                    case 0: // +X
                        View.SetView(Light.Position, FViewVolume::RightAxis, FViewVolume::DownAxis);
                        break;
                    case 1: // -X
                        View.SetView(Light.Position, FViewVolume::LeftAxis, FViewVolume::DownAxis);
                        break;
                    case 2: // +Y
                        View.SetView(Light.Position, FViewVolume::UpAxis, FViewVolume::ForwardAxis);
                        break;
                    case 3: // -Y
                        View.SetView(Light.Position, FViewVolume::DownAxis, FViewVolume::BackwardAxis);
                        break;
                    case 4: // +Z
                        View.SetView(Light.Position, FViewVolume::ForwardAxis, FViewVolume::DownAxis);
                        break;
                    case 5: // -Z
                        View.SetView(Light.Position, FViewVolume::BackwardAxis, FViewVolume::DownAxis);
                        break;
                    default:
                        LUMINA_NO_ENTRY()
                    }
                };

                if (PointLightComponent.bCastShadows)
                {
                    int32 TileIndex = ShadowAtlas.AllocateTile();
                    
                    if (TileIndex != INDEX_NONE)
                    {
                        const FShadowTile& Tile = ShadowAtlas.GetTile(TileIndex);

                        for (int Face = 0; Face < 6; ++Face)
                        {
                            SetView(LightView, Face);
                            
                            Light.ViewProjection[Face]              = LightView.ToReverseDepthViewProjectionMatrix();
                            Light.Shadow[Face].ShadowMapIndex       = TileIndex;
                            Light.Shadow[Face].ShadowMapLayer       = Face;
                            Light.Shadow[Face].AtlasUVOffset        = Tile.UVOffset;
                            Light.Shadow[Face].AtlasUVScale         = Tile.UVScale;
                            Light.Shadow[Face].LightIndex           = (int32)LightData.NumLights;
                        }
                        
                        PackedShadows[(uint32)ELightType::Point].push_back(Light.Shadow[0]);
                    }
                }
                else
                {
                    Light.Shadow[0].ShadowMapIndex = INDEX_NONE;
                }
                
                LightData.Lights[LightData.NumLights++] = Light;
        
                //World->DrawDebugSphere(Light.Position, 0.25f, glm::vec4(PointLightComponent.LightColor, 1.0));
            });
        }
        
        //========================================================================================================================
        
        {
            auto View = World->GetEntityRegistry().view<SSpotLightComponent, STransformComponent>();
            View.each([&] (SSpotLightComponent& SpotLightComponent, STransformComponent& TransformComponent)
            {
                const FTransform& Transform = TransformComponent.WorldTransform;
        
                glm::vec3 UpdatedForward    = Transform.Rotation * FViewVolume::ForwardAxis;
                glm::vec3 UpdatedUp         = Transform.Rotation * FViewVolume::UpAxis;
        
                float InnerDegrees = SpotLightComponent.InnerConeAngle;
                float OuterDegrees = SpotLightComponent.OuterConeAngle;
        
                float InnerCos = glm::cos(glm::radians(InnerDegrees));
                float OuterCos = glm::cos(glm::radians(OuterDegrees));
                
                FViewVolume ViewVolume(OuterDegrees * 2.00f, 1.0f, 0.01f, SpotLightComponent.Attenuation);
                ViewVolume.SetView(Transform.Location, -UpdatedForward, UpdatedUp);
                
                FLight Light;
                Light.Flags                 = LIGHT_TYPE_SPOT;
                Light.Position              = Transform.Location;
                Light.Direction             = glm::normalize(UpdatedForward);
                Light.Falloff               = SpotLightComponent.Falloff;
                Light.Color                 = PackColor(glm::vec4(SpotLightComponent.LightColor, 1.0));
                Light.Intensity             = SpotLightComponent.Intensity;
                Light.Radius                = SpotLightComponent.Attenuation;
                Light.Angles                = glm::vec2(InnerCos, OuterCos);
                Light.ViewProjection[0]     = ViewVolume.ToReverseDepthViewProjectionMatrix();
        
                if (SpotLightComponent.bCastShadows)
                {
                    int32 TileIndex = ShadowAtlas.AllocateTile();
                    if (TileIndex != INDEX_NONE)
                    {
                        const FShadowTile& Tile             = ShadowAtlas.GetTile(TileIndex);
                        Light.Shadow[0].ShadowMapIndex      = TileIndex;
                        Light.Shadow[0].ShadowMapLayer      = 6;
                        Light.Shadow[0].AtlasUVOffset       = Tile.UVOffset;
                        Light.Shadow[0].AtlasUVScale        = Tile.UVScale;
                        Light.Shadow[0].LightIndex          = (int32)LightData.NumLights;

                    }
                    
                    PackedShadows[(uint32)ELightType::Spot].push_back(Light.Shadow[0]);
                }
                else
                {
                    Light.Shadow[0].ShadowMapIndex = INDEX_NONE;
                }
        
                LightData.Lights[LightData.NumLights++] = Light;
                
               //World->DrawViewVolume(ViewVolume, FColor::Red);
        
               //World->DrawDebugCone(SpotLight.Position, Forward, glm::radians(OuterDegrees), SpotLightComponent.Attenuation, glm::vec4(SpotLightComponent.LightColor, 1.0f));
               //World->DrawDebugCone(SpotLight.Position, Forward, glm::radians(InnerDegrees), SpotLightComponent.Attenuation, glm::vec4(SpotLightComponent.LightColor, 1.0f));
        
            });
        }
        
        
        //========================================================================================================================
        
            
        {
            RenderSettings.bHasEnvironment = false;
            LightData.AmbientLight = glm::vec4(0.0f);
            RenderSettings.bSSAO = false;
            auto View = World->GetEntityRegistry().view<SEnvironmentComponent>();
            View.each([this] (const SEnvironmentComponent& EnvironmentComponent)
            {
                LightData.AmbientLight                              = glm::vec4(EnvironmentComponent.AmbientLight.Color, EnvironmentComponent.AmbientLight.Intensity);
                RenderSettings.bHasEnvironment                      = true;
                RenderSettings.bSSAO                                = EnvironmentComponent.bSSAOEnabled;
                SceneGlobalData.SSAOSettings.Intensity              = EnvironmentComponent.SSAOInfo.Intensity;
                SceneGlobalData.SSAOSettings.Power                  = EnvironmentComponent.SSAOInfo.Power;
                SceneGlobalData.SSAOSettings.Radius                 = EnvironmentComponent.SSAOInfo.Radius;
            });
        }
        
        {
            FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
            RenderGraph.AddPass(RG_Raster, FRGEvent("Write Scene Buffers"), Descriptor, [this](ICommandList& CmdList)
            {
                LUMINA_PROFILE_SECTION_COLORED("Write Scene Buffers", tracy::Color::OrangeRed3);
        
                CheckInstanceBufferResize(InstanceData.size());
                CheckLightBufferResize(LightData.NumLights);
                CheckSimpleVertexResize(SimpleVertices.size());
                
                const SIZE_T SimpleVertexSize = SimpleVertices.size() * sizeof(FSimpleElementVertex);
                const SIZE_T InstanceDataSize = InstanceData.size() * sizeof(FInstanceData);
                const SIZE_T IndirectArgsSize = IndirectDrawArguments.size() * sizeof(FDrawIndexedIndirectArguments);
        
                constexpr SIZE_T LightDataHeaderSize = offsetof(FSceneLightData, Lights);
                const SIZE_T ActiveLightsSize = LightData.NumLights * sizeof(FLight);
                const SIZE_T LightUploadSize = LightDataHeaderSize + ActiveLightsSize;
                
                CmdList.SetBufferState(SceneDataBuffer, EResourceStates::CopyDest);
                CmdList.SetBufferState(InstanceDataBuffer, EResourceStates::CopyDest);
                CmdList.SetBufferState(IndirectDrawBuffer, EResourceStates::CopyDest);
                CmdList.SetBufferState(SimpleVertexBuffer, EResourceStates::CopyDest);
                CmdList.SetBufferState(LightDataBuffer, EResourceStates::CopyDest);
                CmdList.CommitBarriers();
                
                CmdList.DisableAutomaticBarriers();
                CmdList.WriteBuffer(SceneDataBuffer, &SceneGlobalData);
                CmdList.WriteBuffer(InstanceDataBuffer, InstanceData.data(), 0, InstanceDataSize);
                CmdList.WriteBuffer(IndirectDrawBuffer, IndirectDrawArguments.data(), 0, IndirectArgsSize);
                CmdList.WriteBuffer(SimpleVertexBuffer, SimpleVertices.data(), 0, SimpleVertexSize);
                CmdList.WriteBuffer(LightDataBuffer, &LightData, 0, LightUploadSize);
                CmdList.EnableAutomaticBarriers();
            });
        }
    }

    void FForwardRenderScene::ResetPass(FRenderGraph& RenderGraph)
    {
        SimpleVertices.clear();
        DrawCommands.clear();
        IndirectDrawArguments.clear();
        InstanceData.clear();
        LightData.NumLights = 0;
        ShadowAtlas.FreeTiles();

        for (int i = 0; i < (int)ELightType::Num; ++i)
        {
            PackedShadows[i].clear();
        }
    }

    void FForwardRenderScene::CullPass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Cull Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cull Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("MeshCull.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(MeshCullLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);

            FCullData CullData;
            CullData.Frustum        = SceneViewport->GetViewVolume().GetFrustum();
            CullData.ViewMatrix     = SceneViewport->GetViewVolume().GetViewMatrix();
            CullData.P00            = SceneViewport->GetViewVolume().GetProjectionMatrix()[0][0];
            CullData.P11            = SceneViewport->GetViewVolume().GetProjectionMatrix()[1][1];
            CullData.zNear          = SceneViewport->GetViewVolume().GetNear();
            CullData.zFar           = SceneViewport->GetViewVolume().GetFar();
            CullData.InstanceNum    = (uint32)InstanceData.size();
            CullData.bFrustumCull   = RenderSettings.bFrustumCull;
            CullData.bOcclusionCull = RenderSettings.bOcclusionCull;
            CullData.PyramidWidth   = (float)DepthPyramid->GetSizeX();
            CullData.PyramidHeight  = (float)DepthPyramid->GetSizeY();

            CmdList.WriteBuffer(CullDataBuffer, &CullData, 0, sizeof(FCullData));
            
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(MeshCullSet);
            CmdList.SetComputeState(State);
            
            uint32 Num = (uint32)InstanceData.size();
            uint32 NumWorkGroups = (Num + 255) / 256;
                
            CmdList.Dispatch(NumWorkGroups, 1, 1);
                
        });
    }

    void FForwardRenderScene::DepthPrePass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Pre-Depth Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Pre-Depth Pass", tracy::Color::Orange);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("DepthPrePass.vert");
        
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(DepthAttachment)
                .SetDepthClearValue(0.0f);
            
            FRenderPassDesc RenderPass; RenderPass
                .SetDepthAttachment(Depth)
                .SetRenderArea(HDRRenderTarget->GetExtent());
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState().SetDepthFunc(EComparisonFunc::Greater))
                .SetRasterState(FRasterState().EnableDepthClip());
                    
            FGraphicsPipelineDesc Desc; Desc
                .SetRenderState(RenderState)
                .SetInputLayout(PositionOnlyLayoutInput)
                .AddBindingLayout(BindingLayout)
                .SetVertexShader(VertexShader);
                
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
            
            for (const FMeshDrawCommand& Batch : DrawCommands)
            {
                FGraphicsState GraphicsState;
                GraphicsState.AddVertexBuffer({ Batch.VertexBuffer });
                GraphicsState.SetIndexBuffer({ Batch.IndexBuffer });
                GraphicsState.SetRenderPass(RenderPass);
                GraphicsState.SetViewportState(MakeViewportStateFromImage(HDRRenderTarget));
                GraphicsState.SetPipeline(Pipeline);
                GraphicsState.AddBindingSet(BindingSet);
                GraphicsState.SetIndirectParams(IndirectDrawBuffer);
                
                CmdList.SetGraphicsState(GraphicsState);
                
                CmdList.DrawIndexedIndirect(1, Batch.IndirectDrawOffset * sizeof(FDrawIndexedIndirectArguments));
            }
        });
    }

    void FForwardRenderScene::DepthPyramidPass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        Descriptor->SetFlag(ERGExecutionFlags::Async);
        RenderGraph.AddPass(RG_Compute, FRGEvent("Depth Pyramid Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Depth Pyramid Pass", tracy::Color::Orange);

            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("DepthPyramidMips.comp");
            int MipCount = DepthPyramid->GetDescription().NumMips;

            CmdList.SetEnableUavBarriersForImage(DepthPyramid, false);

            for (int i = 0; i < MipCount; ++i)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Mip", tracy::Color::Yellow4);

                FBindingLayoutDesc LayoutDesc;
                LayoutDesc.AddItem(FBindingLayoutItem::Texture_SRV(0));
                LayoutDesc.AddItem(FBindingLayoutItem::Texture_UAV(1));
                LayoutDesc.AddItem(FBindingLayoutItem::PushConstants(0, sizeof(glm::vec2)));
                LayoutDesc.SetVisibility(ERHIShaderType::Compute);
                
                FBindingSetDesc SetDesc;
                if (i == 0)
                {
                    SetDesc.AddItem(FBindingSetItem::TextureSRV(0, DepthAttachment));
                }
                else
                {
                    FRHISamplerRef Sampler = TStaticRHISampler<true, false, AM_Clamp, AM_Clamp, AM_Clamp, ESamplerReductionType::Minimum>::GetRHI();
                    SetDesc.AddItem(FBindingSetItem::TextureSRV(0, DepthPyramid, Sampler, DepthPyramid->GetFormat(), FTextureSubresourceSet(i - 1, 1, 0, 1)));
                }

                SetDesc.AddItem(FBindingSetItem::TextureUAV(1, DepthPyramid, DepthPyramid->GetFormat(), FTextureSubresourceSet(i, 1, 0, 1)));
                SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(glm::vec2)));
                
                FRHIBindingLayout* Layout = BindingCache.GetOrCreateBindingLayout(LayoutDesc);
                FRHIBindingSet* Set = BindingCache.GetOrCreateBindingSet(SetDesc, Layout);

                FComputePipelineDesc PipelineDesc;
                PipelineDesc.AddBindingLayout(Layout);
                PipelineDesc.CS = ComputeShader;
                PipelineDesc.DebugName = "Depth Pyramid Mips";

                FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);

                FComputeState State;
                State.AddBindingSet(Set);
                State.SetPipeline(Pipeline);

                CmdList.SetComputeState(State);

                uint32 LevelWidth = DepthPyramid->GetSizeX() >> i;
                uint32 LevelHeight = DepthPyramid->GetSizeY() >> i;

                LevelWidth = std::max(LevelWidth, 1u);
                LevelHeight = std::max(LevelHeight, 1u);
                

                glm::vec2 Data = glm::vec2(LevelWidth,LevelHeight);
                CmdList.SetPushConstants(&Data, sizeof(glm::vec2));

                uint32 GroupsX = RenderUtils::GetGroupCount(LevelWidth, 32);
                uint32 GroupsY = RenderUtils::GetGroupCount(LevelHeight, 32);
                
                CmdList.Dispatch(GroupsX, GroupsY, 1);
            }

            CmdList.SetEnableUavBarriersForImage(DepthPyramid, true);

        });
    }

    void FForwardRenderScene::ClusterBuildPass(FRenderGraph& RenderGraph)
    {
		if (LightData.NumLights == 0)
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Cluster Build Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cluster Build Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("ClusterBuild.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(ClusterBuildLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
                
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(ClusterBuildSet);
            CmdList.SetComputeState(State);

            FLightClusterPC ClusterPC;
            ClusterPC.InverseProjection = SceneViewport->GetViewVolume().GetInverseProjectionMatrix();
            ClusterPC.zNearFar = glm::vec2(SceneViewport->GetViewVolume().GetNear(), SceneViewport->GetViewVolume().GetFar());
            ClusterPC.GridSize = glm::vec4(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ, 0.0f);
            ClusterPC.ScreenSize = glm::vec2(HDRRenderTarget->GetSizeX(), HDRRenderTarget->GetSizeY());
                
            CmdList.SetPushConstants(&ClusterPC, sizeof(FLightClusterPC));
                
            CmdList.Dispatch(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ);
                
        });
    }

    void FForwardRenderScene::LightCullPass(FRenderGraph& RenderGraph)
    {
        if (LightData.NumLights == 0)
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Light Cull Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Light Cull Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("LightCull.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(LightCullLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
                
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(LightCullSet);
            CmdList.SetComputeState(State);
                
            glm::mat4 ViewProj = SceneViewport->GetViewVolume().GetViewMatrix();
                
            CmdList.SetPushConstants(&ViewProj, sizeof(glm::mat4));
                
            CmdList.Dispatch(27, 1, 1);
                
        });
    }

    void FForwardRenderScene::PointShadowPass(FRenderGraph& RenderGraph)
    {
        if (PackedShadows[(uint32)ELightType::Point].empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Point Light Shadow Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Point Light Shadow Pass", tracy::Color::DeepPink2);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ShadowMapping.frag");
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState()
                        .SetSlopeScaleDepthBias(1.75f)
                        .SetDepthBias(100)
                        .SetCullFront());
            
            FGraphicsPipelineDesc Desc; Desc
                .SetDebugName("Point Light Shadow Pass")
                .SetRenderState(RenderState)
                .SetInputLayout(PositionOnlyLayoutInput)
                .AddBindingLayout(BindingLayout)
                .AddBindingLayout(ShadowPassLayout)
                .SetVertexShader(VertexShader)
                .SetPixelShader(PixelShader);

            FRenderPassDesc::FAttachment Depth; Depth
                .SetLoadOp(ERenderLoadOp::Clear)
                .SetDepthClearValue(1.0)
                .SetImage(ShadowAtlas.GetImage())
                    .SetNumSlices(6);
            
            FRenderPassDesc RenderPass; RenderPass
                .SetDepthAttachment(Depth)
                .SetViewMask(RenderUtils::CreateViewMask<0u, 1u, 2u, 3u, 4u, 5u>())
                .SetRenderArea(glm::uvec2(GShadowAtlasResolution, GShadowAtlasResolution));
            
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
            
            FGraphicsState GraphicsState; GraphicsState
                .SetRenderPass(Move(RenderPass))
                .AddBindingSet(BindingSet)
                .AddBindingSet(ShadowPassSet)
                .SetIndirectParams(IndirectDrawBuffer)
                .SetPipeline(Pipeline);

            const TVector<FLightShadow>& PointShadows = PackedShadows[(uint32)ELightType::Point];
            

            for (size_t Shadow = 0; Shadow < PointShadows.size(); Shadow++)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Point Light", tracy::Color::DeepPink2);

                const FLightShadow& LightShadow = PointShadows[Shadow];
                
                const FShadowTile& Tile = ShadowAtlas.GetTile(LightShadow.ShadowMapIndex);
                uint32 TilePixelX       = static_cast<uint32>(Tile.UVOffset.x * GShadowAtlasResolution);
                uint32 TilePixelY       = static_cast<uint32>(Tile.UVOffset.y * GShadowAtlasResolution);
                uint32 TileSize         = static_cast<uint32>(Tile.UVScale.x * GShadowAtlasResolution);
                
                FViewport Viewport
                (
                    (float)TilePixelX,
                    (float)TilePixelX + TileSize,
                    (float)TilePixelY,
                    (float)TilePixelY + TileSize,
                    0.0f,
                    1.0f 
                );
                
                // FRect(minX, maxX, minY, maxY)
                FRect Scissor
                (
                    (int)TilePixelX,
                    (int)TilePixelX + TileSize,
                    (int)TilePixelY,
                    (int)TilePixelY + TileSize
                );

                GraphicsState.SetViewportState(FViewportState(Viewport, Scissor));
                
                for (SIZE_T CurrentDraw = 0; CurrentDraw < DrawCommands.size(); ++CurrentDraw)
                {
                    const FMeshDrawCommand& Batch = DrawCommands[CurrentDraw];

                    GraphicsState.SetVertexBuffer({ Batch.VertexBuffer });
                    GraphicsState.SetIndexBuffer({ Batch.IndexBuffer });
                    
                    CmdList.SetGraphicsState(GraphicsState);
                    
                    CmdList.SetPushConstants(&LightShadow.LightIndex, sizeof(uint32));
                    CmdList.DrawIndexedIndirect(1, Batch.IndirectDrawOffset * sizeof(FDrawIndexedIndirectArguments));
                }
            }

            CmdList.EndRenderPass();
        });
    }

    void FForwardRenderScene::SpotShadowPass(FRenderGraph& RenderGraph)
    {
        if (PackedShadows[(uint32)ELightType::Spot].empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Spot Shadow Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Spot Shadow Pass", tracy::Color::DeepPink4);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ShadowMapping.frag");
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState()
                        .SetSlopeScaleDepthBias(1.75f)
                        .SetDepthBias(100)
                        .SetCullFront());
            
            
            FGraphicsPipelineDesc Desc; Desc
                .SetDebugName("Spot Shadow Pass")
                .SetRenderState(RenderState)
                .SetInputLayout(PositionOnlyLayoutInput)
                .AddBindingLayout(BindingLayout)
                .AddBindingLayout(ShadowPassLayout)
                .SetVertexShader(VertexShader)
                .SetPixelShader(PixelShader);
            
            const TVector<FLightShadow>& SpotShadows = PackedShadows[(uint32)ELightType::Spot];

            for (int i = 0; i < (int)SpotShadows.size(); ++i)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Spot Light", tracy::Color::DeepPink);

                const FLightShadow& Shadow = SpotShadows[i];
                const FShadowTile& Tile = ShadowAtlas.GetTile(Shadow.ShadowMapIndex);
                uint32 TilePixelX = static_cast<uint32>(Tile.UVOffset.x * GShadowAtlasResolution);
                uint32 TilePixelY = static_cast<uint32>(Tile.UVOffset.y * GShadowAtlasResolution);
                uint32 TileSize = static_cast<uint32>(Tile.UVScale.x * GShadowAtlasResolution);
                
                FRenderPassDesc::FAttachment Depth; Depth
                    .SetLoadOp(ERenderLoadOp::Clear)
                    .SetDepthClearValue(1.0f)
                    .SetImage(ShadowAtlas.GetImage())
                        .SetArraySlice(6);
                
                FRenderPassDesc RenderPass; RenderPass
                    .SetDepthAttachment(Depth)
                    .SetRenderArea(glm::uvec2(GShadowAtlasResolution, GShadowAtlasResolution));
        
                FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
                
                FViewportState ViewportState;
                
                ViewportState.Viewports.emplace_back(FViewport
                (
                    (float)TilePixelX,
                    (float)TilePixelX + TileSize,
                    (float)TilePixelY,
                    (float)TilePixelY + TileSize,
                    0.0f,
                    1.0f 
                ));
        
                // FRect(minX, maxX, minY, maxY)
                ViewportState.Scissors.emplace_back(FRect
                (
                    (int)TilePixelX,
                    (int)TilePixelX + TileSize,
                    (int)TilePixelY,
                    (int)TilePixelY + TileSize
                ));
                
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .SetViewportState(ViewportState)
                    .SetPipeline(Pipeline)
                    .AddBindingSet(BindingSet)
                    .AddBindingSet(ShadowPassSet)
                    .SetIndirectParams(IndirectDrawBuffer);                    
                
                
                for (SIZE_T CurrentDraw = 0; CurrentDraw < DrawCommands.size(); ++CurrentDraw)
                {
                    const FMeshDrawCommand& Batch = DrawCommands[CurrentDraw];
        
                    GraphicsState.SetVertexBuffer({Batch.VertexBuffer});
                    GraphicsState.SetIndexBuffer({Batch.IndexBuffer});
                    CmdList.SetGraphicsState(GraphicsState);
                    
                    CmdList.SetPushConstants(&Shadow.LightIndex, sizeof(uint32));
                    CmdList.DrawIndexedIndirect(1, Batch.IndirectDrawOffset * sizeof(FDrawIndexedIndirectArguments));
                }
            }

            CmdList.EndRenderPass();

        });
    }

    void FForwardRenderScene::DirectionalShowPass(FRenderGraph& RenderGraph)
    {
        if (!LightData.bHasSun)
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Cascaded Shadow Map Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cascaded Shadow Map Pass", tracy::Color::DeepPink2);
        
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");
            //FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ShadowMapping.frag");
        
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState().SetSlopeScaleDepthBias(1.75f).SetDepthBias(100).SetCullFront());
                        
            FGraphicsPipelineDesc Desc; Desc
                .SetDebugName("Cascaded Shadow Maps")
                .SetRenderState(RenderState)
                .SetInputLayout(PositionOnlyLayoutInput)
                .AddBindingLayout(BindingLayout)
                .AddBindingLayout(ShadowPassLayout)
                .SetVertexShader(VertexShader);
                //.SetPixelShader(PixelShader);
        
            
            for (int i = 0; i < NumCascades; ++i)
            {
                FShadowCascade& Cascade = ShadowCascades[i];
        
                FRenderPassDesc::FAttachment Depth; Depth
                    .SetImage(CascadedShadowMap)
                    .SetDepthClearValue(1.0f);
            
                FRenderPassDesc RenderPass; RenderPass
                    .SetDepthAttachment(Depth, FTextureSubresourceSet(0, 1, i, 1))
                    .SetRenderArea(glm::uvec2(Cascade.ShadowMapSize.x, Cascade.ShadowMapSize.y));
        
                FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
                
                for (SIZE_T CurrentDraw = 0; CurrentDraw < DrawCommands.size(); ++CurrentDraw)
                {
                    const FMeshDrawCommand& Batch = DrawCommands[CurrentDraw];
        
                    FGraphicsState GraphicsState; GraphicsState
                        .SetRenderPass(RenderPass)
                        .SetViewportState(MakeViewportStateFromImage(CascadedShadowMap))
                        .SetPipeline(Pipeline)
                        .AddBindingSet(BindingSet)
                        .AddBindingSet(ShadowPassSet)
                        .SetIndirectParams(IndirectDrawBuffer)
                        .AddVertexBuffer({Batch.VertexBuffer})
                        .SetIndexBuffer({Batch.IndexBuffer});
                    
                    CmdList.SetGraphicsState(GraphicsState);
        
                    uint32 LightIndex = 0;
                    CmdList.SetPushConstants(&LightIndex, sizeof(uint32));
                    CmdList.DrawIndexedIndirect(1, Batch.IndirectDrawOffset * sizeof(FDrawIndexedIndirectArguments));
                }
            }
        });
    }

    void FForwardRenderScene::BasePass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty() || !CVarRenderBasePass)
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Forward Base Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Forward Base Pass", tracy::Color::Red);
            
            FRenderPassDesc::FAttachment RenderTarget;
            RenderTarget.SetImage(HDRRenderTarget);
            if (RenderSettings.bHasEnvironment)
            {
                RenderTarget.SetLoadOp(ERenderLoadOp::Load);
            }
            
            FRenderPassDesc::FAttachment PickerImageAttachment; PickerImageAttachment
                .SetImage(PickerImage);
            
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(DepthAttachment)
                .SetLoadOp(ERenderLoadOp::Load);
            
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(RenderTarget)
                .AddColorAttachment(PickerImageAttachment)
                .SetDepthAttachment(Depth)
                .SetRenderArea(HDRRenderTarget->GetExtent());
            
            
            FRasterState RasterState;
            RasterState.EnableDepthClip();
        
            FDepthStencilState DepthState; DepthState
                .SetDepthFunc(EComparisonFunc::Equal)
                .DisableDepthWrite();

            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
            
            for (SIZE_T CurrentDraw = 0; CurrentDraw < DrawCommands.size(); ++CurrentDraw)
            {
                const FMeshDrawCommand& Batch = DrawCommands[CurrentDraw];
                CMaterialInterface* Mat = Batch.Material;
        
                FGraphicsPipelineDesc Desc; Desc
                    .SetDebugName("Forward Base Pass")
                    .SetRenderState(RenderState)
                    .SetInputLayout(VertexLayoutInput)
                    .AddBindingLayout(BindingLayout)
                    .AddBindingLayout(BasePassLayout)
                    .AddBindingLayout(Mat->GetBindingLayout())
                    .SetVertexShader(Mat->GetMaterial()->VertexShader)
                    .SetPixelShader(Mat->GetMaterial()->PixelShader);
                
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .AddVertexBuffer({ Batch.VertexBuffer })
                    .SetIndexBuffer({ Batch.IndexBuffer })
                    .SetViewportState(MakeViewportStateFromImage(HDRRenderTarget))
                    .SetPipeline(GRenderContext->CreateGraphicsPipeline(Desc, RenderPass))
                    .SetIndirectParams(IndirectDrawBuffer)
                    .AddBindingSet(BindingSet)
                    .AddBindingSet(BasePassSet)
                    .AddBindingSet(Mat->GetBindingSet());
                
                CmdList.SetGraphicsState(GraphicsState);
                CmdList.DrawIndexedIndirect(1, Batch.IndirectDrawOffset * sizeof(FDrawIndexedIndirectArguments));
            }
        });
    }

    void FForwardRenderScene::TransparentPass(FRenderGraph& RenderGraph)
    {
        
    }

    void FForwardRenderScene::EnvironmentPass(FRenderGraph& RenderGraph)
    {
        if (!RenderSettings.bHasEnvironment)
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Environment Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Environment Pass", tracy::Color::Green3);
        
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("Environment.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
        
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetImage(HDRRenderTarget);
            
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(HDRRenderTarget->GetExtent());
        
            FRasterState RasterState;
            RasterState.SetCullNone();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
        
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Environment Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(BindingLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.AddBindingSet(BindingSet);
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.SetRenderPass(RenderPass);
            GraphicsState.SetViewportState(MakeViewportStateFromImage(HDRRenderTarget));
        
            CmdList.SetGraphicsState(GraphicsState);
        
            CmdList.Draw(3, 1, 0, 0); 
        });
    }

    void FForwardRenderScene::BatchedLineDraw(FRenderGraph& RenderGraph)
    {
        if (SimpleVertices.empty())
        {
            return;
		}

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Batched Line Draw"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Batched Line Draw", tracy::Color::Red2);

            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("SimpleElement.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("SimpleElement.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }

            FRenderPassDesc::FAttachment RenderTarget;
            RenderTarget.SetImage(HDRRenderTarget);
            if (!DrawCommands.empty())
            {
                RenderTarget.SetLoadOp(ERenderLoadOp::Load);
            }

            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(DepthAttachment)
                .SetLoadOp(ERenderLoadOp::Load);

            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(RenderTarget)
                .SetDepthAttachment(Depth)
                .SetRenderArea(HDRRenderTarget->GetExtent());
            
            FRasterState RasterState; RasterState
                .SetLineWidth(3.5f)
                .EnableDepthClip();

            FDepthStencilState DepthState; DepthState
                .SetDepthFunc(EComparisonFunc::Greater)
                .EnableDepthWrite()
                .EnableDepthTest();

            FRenderState RenderState; RenderState
                .SetRasterState(RasterState)
                .SetDepthStencilState(DepthState);
            
            FGraphicsPipelineDesc Desc; Desc
                .SetDebugName("Batched Line Draw")
                .SetPrimType(EPrimitiveType::LineList)
                .SetRenderState(RenderState)
                .SetInputLayout(SimpleVertexLayoutInput)
                .AddBindingLayout(BindingLayout)
                .AddBindingLayout(BasePassLayout)
                .SetVertexShader(VertexShader)
                .SetPixelShader(PixelShader);

            FGraphicsState GraphicsState; GraphicsState
                .SetRenderPass(RenderPass)
                .AddVertexBuffer(FVertexBufferBinding{SimpleVertexBuffer})
                .SetViewportState(MakeViewportStateFromImage(HDRRenderTarget))
                .SetPipeline(GRenderContext->CreateGraphicsPipeline(Desc, RenderPass))
                .AddBindingSet(BindingSet)
                .AddBindingSet(BasePassSet);

            CmdList.SetGraphicsState(GraphicsState);
            CmdList.Draw(SimpleVertices.size(), 1, 0, 0);
            
        });
    }

    void FForwardRenderScene::ToneMappingPass(FRenderGraph& RenderGraph)
    {
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Tone Mapping Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Tone Mapping Pass", tracy::Color::Red2);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ToneMapping.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
            
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetImage(GetRenderTarget());
        
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetRenderTarget()->GetExtent());
        
        
            FRasterState RasterState;
            RasterState.SetCullNone();
            
            FDepthStencilState DepthState;
            DepthState.DisableDepthTest();
            DepthState.DisableDepthWrite();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
            
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Tone Mapping Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(ToneMappingPassLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.AddBindingSet(ToneMappingPassSet);
            GraphicsState.SetRenderPass(RenderPass);               
            GraphicsState.SetViewportState(MakeViewportStateFromImage(GetRenderTarget()));
        
            CmdList.SetGraphicsState(GraphicsState);

            glm::vec2 PC;
            PC.x = 1.0;
            PC.y = SceneGlobalData.Time;
            CmdList.SetPushConstants(&PC, sizeof(glm::vec2));
            CmdList.Draw(3, 1, 0, 0); 
        });
    }

    void FForwardRenderScene::DebugDrawPass(FRenderGraph& RenderGraph)
    {
        if (DebugVisualizationMode == ERenderSceneDebugFlags::None)
        {
            return;
        }


        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Debug Draw Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Debug Draw Pass", tracy::Color::Red2);
        
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("Debug.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
        
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetLoadOp(ERenderLoadOp::Load)
                .SetImage(GetRenderTarget());
        
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetRenderTarget()->GetExtent());
        
        
            FRasterState RasterState;
            RasterState.SetCullNone();
        
            FDepthStencilState DepthState;
            DepthState.DisableDepthTest();
            DepthState.DisableDepthWrite();
        
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
        
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Debug Draw Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(BindingLayout);
            Desc.AddBindingLayout(DebugPassLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.AddBindingSet(BindingSet);
            GraphicsState.AddBindingSet(DebugPassSet);
            GraphicsState.SetRenderPass(RenderPass);               
            GraphicsState.SetViewportState(MakeViewportStateFromImage(GetRenderTarget()));
        
            CmdList.SetGraphicsState(GraphicsState);
        
            uint32 Mode = (uint32)DebugVisualizationMode;
            CmdList.SetPushConstants(&Mode, sizeof(uint32));
            CmdList.Draw(3, 1, 0, 0); 
        });
    }
    
    void FForwardRenderScene::InitBuffers()
    {
        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FSceneGlobalData);
            BufferDesc.Usage.SetFlag(BUF_UniformBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Scene Global Data";
            SceneDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FInstanceData) * 1'000;
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Instance Buffer";
            InstanceDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(uint32) * 1'000;
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::UnorderedAccess;
            BufferDesc.DebugName = "Instance Mapping";
            InstanceMappingBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = offsetof(FSceneLightData, Lights);
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Light Data Buffer";
            LightDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FCluster) * NumClusters;
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::UnorderedAccess;
            BufferDesc.DebugName = "Cluster SSBO";
            ClusterBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FCullData);
            BufferDesc.Usage.SetFlag(BUF_UniformBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Cull Data Buffer";
            CullDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            SimpleVertexBuffer = FRHITypedVertexBuffer<FSimpleElementVertex>::CreateEmpty(1'000);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FDrawIndexedIndirectArguments) * (500);
            BufferDesc.Stride = sizeof(FDrawIndexedIndirectArguments);
            BufferDesc.Usage.SetMultipleFlags(BUF_Indirect, BUF_StorageBuffer);
            BufferDesc.InitialState = EResourceStates::IndirectArgument;
            BufferDesc.bKeepInitialState = true;
            BufferDesc.DebugName = "Indirect Draw Buffer";
            IndirectDrawBuffer = GRenderContext->CreateBuffer(BufferDesc);
        }
    }

    static uint32 PreviousPow2(uint32 v)
    {
        uint32_t r = 1;

        while (r * 2 < v)
        {
            r *= 2;
        }

        return r;
    }

    void FForwardRenderScene::InitImages()
    {
        glm::uvec2 Extent = Windowing::GetPrimaryWindowHandle()->GetExtent();
        
        {
            FRHIImageDesc ImageDesc = GetRenderTarget()->GetDescription();
            ImageDesc.Format = EFormat::RGBA16_FLOAT;
            HDRRenderTarget = GRenderContext->CreateImage(ImageDesc);
        }
        
        
        {
            FRHIImageDesc ImageDesc;
            ImageDesc.Extent = Extent;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::DepthAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.Format = EFormat::D32;
            ImageDesc.InitialState = EResourceStates::DepthRead;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.DebugName = "Depth Attachment";
        
            DepthAttachment = GRenderContext->CreateImage(ImageDesc);
        }

        //==================================================================================================

        {
            uint32 Width = PreviousPow2(Extent.x);
            uint32 Height = PreviousPow2(Extent.y);
            
            FRHIImageDesc ImageDesc;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::ShaderResource, EImageCreateFlags::Storage);
            ImageDesc.Extent            = glm::uvec2(Width, Height);
            ImageDesc.Format            = EFormat::R32_FLOAT;
            ImageDesc.NumMips           = (uint8)RenderUtils::CalculateMipCount(Width, Height);
            ImageDesc.InitialState      = EResourceStates::ShaderResource;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Dimension         = EImageDimension::Texture2D;
            ImageDesc.DebugName         = "Depth Pyramid";
            
            DepthPyramid                = GRenderContext->CreateImage(ImageDesc);
        }

        //==================================================================================================
        
        {
            FRHIImageDesc ImageDesc;
            ImageDesc.Extent = Extent;
            ImageDesc.Format = EFormat::R32_UINT;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.InitialState = EResourceStates::RenderTarget;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::ColorAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.DebugName = "Picker";
            
            PickerImage = GRenderContext->CreateImage(ImageDesc);
        }
        
        //==================================================================================================
        
        {
            FRHIImageDesc ImageDesc = {};
            ImageDesc.Extent = glm::uvec2(GCSMResolution, GCSMResolution);
            ImageDesc.Format = EFormat::D32;
            ImageDesc.Dimension = EImageDimension::Texture2DArray;
            ImageDesc.InitialState = EResourceStates::DepthWrite;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.ArraySize = NumCascades;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::DepthAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.DebugName = "ShadowCascadeMap";
            
            CascadedShadowMap = GRenderContext->CreateImage(ImageDesc);
        }
        
        //==================================================================================================
        
    }

    void FForwardRenderScene::InitFrameResources()
    {
        InitImages();
        
        {
            FVertexAttributeDesc VertexDesc[4];
            // Pos
            VertexDesc[0].SetElementStride(sizeof(FVertex));
            VertexDesc[0].SetOffset(offsetof(FVertex, Position));
            VertexDesc[0].Format = EFormat::RGB32_FLOAT;
        
            // Normal
            VertexDesc[1].SetElementStride(sizeof(FVertex));
            VertexDesc[1].SetOffset(offsetof(FVertex, Normal));
            VertexDesc[1].Format = EFormat::R32_UINT;
        
            // UV
            VertexDesc[2].SetElementStride(sizeof(FVertex));
            VertexDesc[2].SetOffset(offsetof(FVertex, UV));
            VertexDesc[2].Format = EFormat::RG16_UINT;
            
            // Color
            VertexDesc[3].SetElementStride(sizeof(FVertex));
            VertexDesc[3].SetOffset(offsetof(FVertex, Color));
            VertexDesc[3].Format = EFormat::RGBA8_UNORM;
        
        
            VertexLayoutInput = GRenderContext->CreateInputLayout(VertexDesc, std::size(VertexDesc));
        }
        
        {
            FVertexAttributeDesc VertexDesc[1];
            // Pos
            VertexDesc[0].SetElementStride(sizeof(FVertex));
            VertexDesc[0].SetOffset(offsetof(FVertex, Position));
            VertexDesc[0].Format = EFormat::RGB32_FLOAT;
            
            PositionOnlyLayoutInput = GRenderContext->CreateInputLayout(VertexDesc, 1);
        }
        
        {
            FVertexAttributeDesc VertexDesc[2];
            // Pos
            VertexDesc[0].SetElementStride(sizeof(FSimpleElementVertex));
            VertexDesc[0].SetOffset(offsetof(FSimpleElementVertex, Position));
            VertexDesc[0].Format = EFormat::RGBA32_FLOAT;
        
            // Color
            VertexDesc[1].SetElementStride(sizeof(FSimpleElementVertex));
            VertexDesc[1].SetOffset(offsetof(FSimpleElementVertex, Color));
            VertexDesc[1].Format = EFormat::RGBA32_FLOAT;
        
            SimpleVertexLayoutInput = GRenderContext->CreateInputLayout(VertexDesc, std::size(VertexDesc));
        }
        
        {
            FBindingSetDesc BindingSetDesc;
            BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(0, InstanceDataBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(1, InstanceMappingBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(2, IndirectDrawBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(3, CullDataBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(4, DepthPyramid, TStaticRHISampler<true, false, AM_Clamp, AM_Clamp, AM_Clamp, ESamplerReductionType::Minimum>::GetRHI()));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Compute);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, MeshCullLayout, MeshCullSet);
        }
        
        {
        
            FBindingSetDesc BindingSetDesc;
            BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(0, SceneDataBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(1, InstanceDataBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(2, InstanceMappingBuffer));
            BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(3, LightDataBuffer));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Vertex, ERHIShaderType::Fragment);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, BindingLayout, BindingSet);
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::TextureSRV(0, CascadedShadowMap));
            SetDesc.AddItem(FBindingSetItem::TextureSRV(1, ShadowAtlas.GetImage(), TStaticRHISampler<true, true, AM_Border, AM_Border, AM_Border>::GetRHI()));
            SetDesc.AddItem(FBindingSetItem::BufferSRV(2, ClusterBuffer));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Fragment);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, BasePassLayout, BasePassSet);
        
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::TextureSRV(0, DepthAttachment));
            SetDesc.AddItem(FBindingSetItem::TextureSRV(1, ShadowAtlas.GetImage()));
        
            SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(uint32)));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Fragment);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, DebugPassLayout, DebugPassSet);
        }
        
        {
            FBindingLayoutDesc LayoutDesc;
            LayoutDesc.StageFlags.SetFlag(ERHIShaderType::Vertex);
            LayoutDesc.AddItem(FBindingLayoutItem::PushConstants(0, 80));
            SimplePassLayout = GRenderContext->CreateBindingLayout(LayoutDesc);
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::BufferUAV(0, ClusterBuffer));
            SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(FLightClusterPC)));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Compute);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, ClusterBuildLayout, ClusterBuildSet);
        
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::BufferUAV(0, ClusterBuffer));
            SetDesc.AddItem(FBindingSetItem::BufferUAV(1, LightDataBuffer));
            SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(glm::mat4)));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Compute);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, LightCullLayout, LightCullSet);
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::TextureSRV(0, HDRRenderTarget));
            SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(glm::vec2)));

            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Fragment);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, ToneMappingPassLayout, ToneMappingPassSet);
        }
        
        {
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(uint32)));
        
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Vertex, ERHIShaderType::Fragment);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, ShadowPassLayout, ShadowPassSet);
        
        }
        
    }

    FViewportState FForwardRenderScene::MakeViewportStateFromImage(const FRHIImage* Image)
    {
        float SizeY = (float)Image->GetSizeY();
        float SizeX = (float)Image->GetSizeX();

        FViewportState ViewportState;
        ViewportState.Viewports.emplace_back(FViewport(SizeX, SizeY));
        ViewportState.Scissors.emplace_back(FRect(SizeX, SizeY));

        return ViewportState;
    }

    void FForwardRenderScene::CheckInstanceBufferResize(uint32 NumInstances)
    {
        uint32 SizeRequiredBytes = NumInstances * sizeof(FInstanceData);
        uint32 NumToReallocate = NumInstances * 2;
        
        if (InstanceDataBuffer->GetDescription().Size < SizeRequiredBytes)
        {
            {
                FRHIBufferDesc BufferDesc;
                BufferDesc.Size = sizeof(FInstanceData) * NumToReallocate;
                BufferDesc.Usage.SetMultipleFlags(BUF_StorageBuffer);
                BufferDesc.bKeepInitialState = true;
                BufferDesc.InitialState = EResourceStates::ShaderResource;
                BufferDesc.DebugName = "Instance Buffer";
                InstanceDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
            }
        
        
            {
                FRHIBufferDesc BufferDesc;
                BufferDesc.Size = sizeof(uint32) * NumToReallocate;
                BufferDesc.Usage.SetMultipleFlags(BUF_StorageBuffer);
                BufferDesc.bKeepInitialState = true;
                BufferDesc.InitialState = EResourceStates::UnorderedAccess;
                BufferDesc.DebugName = "Instance Mapping";
                InstanceMappingBuffer = GRenderContext->CreateBuffer(BufferDesc);
            }
        
        
            {
                FBindingSetDesc BindingSetDesc;
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(0, InstanceDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(1, InstanceMappingBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(2, IndirectDrawBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(3, CullDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(4, DepthPyramid));
        
                TBitFlags<ERHIShaderType> Visibility;
                Visibility.SetMultipleFlags(ERHIShaderType::Compute);
                GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, MeshCullLayout, MeshCullSet);
            }
        
            {
        
                FBindingSetDesc BindingSetDesc;
                BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(0, SceneDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(1, InstanceDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(2, InstanceMappingBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(3, LightDataBuffer));
        
                TBitFlags<ERHIShaderType> Visibility;
                Visibility.SetMultipleFlags(ERHIShaderType::Vertex, ERHIShaderType::Fragment);
                GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, BindingLayout, BindingSet);
            }
        }
    }

    void FForwardRenderScene::CheckLightBufferResize(uint32 NumLights)
    {
        constexpr SIZE_T LightDataHeaderSize = offsetof(FSceneLightData, Lights);
        const SIZE_T ActiveLightsSize = NumLights * sizeof(FLight);
        const SIZE_T LightUploadSize = LightDataHeaderSize + ActiveLightsSize;
        
        if (LightDataBuffer->GetDescription().Size < LightUploadSize)
        {
            {
                FRHIBufferDesc BufferDesc;
                BufferDesc.Size = LightUploadSize * 2;
                BufferDesc.Usage.SetMultipleFlags(BUF_StorageBuffer);
                BufferDesc.bKeepInitialState = true;
                BufferDesc.InitialState = EResourceStates::ShaderResource;
                BufferDesc.DebugName = "Light Data Buffer";
                LightDataBuffer = GRenderContext->CreateBuffer(BufferDesc);
            }
        
            
            {
        
                FBindingSetDesc BindingSetDesc;
                BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(0, SceneDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(1, InstanceDataBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(2, InstanceMappingBuffer));
                BindingSetDesc.AddItem(FBindingSetItem::BufferSRV(3, LightDataBuffer));
        
                TBitFlags<ERHIShaderType> Visibility;
                Visibility.SetMultipleFlags(ERHIShaderType::Vertex, ERHIShaderType::Fragment);
                GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, BindingLayout, BindingSet);
            }
        
            
            {
                FBindingSetDesc SetDesc;
                SetDesc.AddItem(FBindingSetItem::BufferUAV(0, ClusterBuffer));
                SetDesc.AddItem(FBindingSetItem::BufferUAV(1, LightDataBuffer));
                SetDesc.AddItem(FBindingSetItem::PushConstants(0, sizeof(glm::mat4)));
        
                TBitFlags<ERHIShaderType> Visibility;
                Visibility.SetMultipleFlags(ERHIShaderType::Compute);
                GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, LightCullLayout, LightCullSet);
            }
            
        }
    }

    void FForwardRenderScene::CheckSimpleVertexResize(uint32 NumVertices)
    {
        uint32 SizeRequiredBytes = NumVertices * sizeof(FSimpleElementVertex);
        uint32 NumToReallocate = NumVertices * 2;
        
        if (SimpleVertexBuffer->GetDescription().Size < SizeRequiredBytes)
        {
            SimpleVertexBuffer = FRHITypedVertexBuffer<FSimpleElementVertex>::CreateEmpty(NumToReallocate);
        }
    }

    FRHIImageRef FForwardRenderScene::GetRenderTarget() const
    {
        return SceneViewport->GetRenderTarget();
    }

    ERenderSceneDebugFlags FForwardRenderScene::GetDebugMode() const
    {
        return DebugVisualizationMode;
    }

    void FForwardRenderScene::SetDebugMode(ERenderSceneDebugFlags Mode)
    {
        DebugVisualizationMode = Mode;
    }

    FSceneRenderSettings& FForwardRenderScene::GetSceneRenderSettings()
    {
        return RenderSettings;
    }

    entt::entity FForwardRenderScene::GetEntityAtPixel(uint32 X, uint32 Y) const
    {
        if (!PickerImage)
        {
            return entt::null;
        }

        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        CommandList->Open();

        FRHIStagingImageRef StagingImage = GRenderContext->CreateStagingImage(PickerImage->GetDescription(), ERHIAccess::HostRead);
        CommandList->CopyImage(PickerImage, FTextureSlice(), StagingImage, FTextureSlice());

        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList);

        size_t RowPitch = 0;
        void* MappedMemory = GRenderContext->MapStagingTexture(StagingImage, FTextureSlice(), ERHIAccess::HostRead, &RowPitch);
        if (!MappedMemory)
        {
            return entt::null;
        }

        const uint32 Width  = PickerImage->GetDescription().Extent.x;
        const uint32 Height = PickerImage->GetDescription().Extent.y;

        if (X >= Width || Y >= Height)
        {
            GRenderContext->UnMapStagingTexture(StagingImage);
            return entt::null;
        }

        uint8* RowStart = reinterpret_cast<uint8*>(MappedMemory) + Y * RowPitch;
        uint32* PixelPtr = reinterpret_cast<uint32*>(RowStart) + X;
        uint32 PixelValue = *PixelPtr;

        GRenderContext->UnMapStagingTexture(StagingImage);

        if (PixelValue == 0)
        {
            return entt::null;
        }

        return static_cast<entt::entity>(PixelValue);
    }
}
