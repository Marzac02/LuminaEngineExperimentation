#include "WorldEditorTool.h"
#include "EditorToolContext.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Object/Package/Package.h"
#include "EASTL/sort.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "Paths/Paths.h"
#include "Tools/ComponentVisualizers/ComponentVisualizer.h"
#include "Tools/Dialogs/Dialogs.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "World/WorldManager.h"
#include "World/Entity/EntityUtils.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/EditorComponent.h"
#include "World/Entity/Components/NameComponent.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/Entity/Components/TagComponent.h"
#include "World/Entity/Components/VelocityComponent.h"
#include "World/Entity/Systems/EditorEntityMovementSystem.h"
#include "World/Scene/RenderScene/RenderScene.h"
#include "World/Scene/RenderScene/SceneRenderTypes.h"


namespace Lumina
{
    static constexpr const char* SystemOutlinerName = "Systems";
    static constexpr const char* WorldSettingsName = "WorldSettings";

    FWorldEditorTool::FWorldEditorTool(IEditorToolContext* Context, CWorld* InWorld)
        : FEditorTool(Context, InWorld->GetName().ToString(), InWorld)
        , SelectedEntity(entt::null)
        , CopiedEntity(entt::null)
    {
        GuizmoOp = ImGuizmo::TRANSLATE;
        GuizmoMode = ImGuizmo::WORLD;
    }

    void FWorldEditorTool::OnInitialize()
    {
        CreateToolWindow("Outliner", [&] (bool bFocused)
        {
            DrawOutliner(bFocused);
        });
        
        CreateToolWindow(WorldSettingsName, [&](bool bFocused)
        {
            
        });

        CreateToolWindow(SystemOutlinerName, [&] (bool bFocused)
        {
            DrawSystems(bFocused);
        });
        
        CreateToolWindow("Details", [&] (bool bFocused)
        {
            DrawEntityEditor(bFocused, SelectedEntity);
        });


        //------------------------------------------------------------------------------------------------------

        OutlinerContext.DrawItemContextMenuFunction = [this](const TVector<FTreeListViewItem*>& Items)
        {
            for (FTreeListViewItem* Item : Items)
            {
                FEntityListViewItem* EntityListItem = static_cast<FEntityListViewItem*>(Item);

                if (ImGui::MenuItem("Add Component"))
                {
                    PushAddComponentModal(EntityListItem->GetEntity());
                }
                
                if (ImGui::MenuItem("Rename"))
                {
                    PushRenameEntityModal(EntityListItem->GetEntity());
                }

                if (ImGui::MenuItem("Duplicate"))
                {
                    entt::entity New = entt::null;
                    CopyEntity(New, SelectedEntity);
                }
                
                if (ImGui::MenuItem("Delete"))
                {
                    EntityDestroyRequests.push(EntityListItem->GetEntity());
                }
            }
        };

        OutlinerContext.RebuildTreeFunction = [this](FTreeListView* Tree)
        {
            RebuildSceneOutliner(Tree);
        };
        
        OutlinerContext.ItemSelectedFunction = [this](FTreeListViewItem* Item)
        {
            if (Item == nullptr)
            {
                SelectedEntity = entt::null;
                return;
            }
            
            FEntityListViewItem* EntityListItem = static_cast<FEntityListViewItem*>(Item);
            
            SelectedEntity = EntityListItem->GetEntity();
            
            RebuildPropertyTables(SelectedEntity);
        };

        OutlinerContext.DragDropFunction = [this](FTreeListViewItem* Item)
        {
            HandleEntityEditorDragDrop(Item);  
        };

        OutlinerContext.FilterFunc = [this](const FTreeListViewItem* Item) -> bool
        {
            return EntityFilterState.FilterName.PassFilter(Item->GetName().c_str());
        };
        
        //------------------------------------------------------------------------------------------------------
        
        
        World->GetEntityRegistry().on_destroy<entt::entity>().connect<&FWorldEditorTool::OnEntityDestroyed>(this);
    }

    void FWorldEditorTool::OnDeinitialize(const FUpdateContext& UpdateContext)
    {
        if (bSimulatingWorld)
        {
            SetWorldNewSimulate(false);
        }
        
        if (bGamePreviewRunning)
        {
            OnGamePreviewStopRequested.Broadcast();
        }
    }

    void FWorldEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        while (!ComponentDestroyRequests.empty())
        {
            FComponentDestroyRequest Request = ComponentDestroyRequests.back();
            ComponentDestroyRequests.pop();
            
            RemoveComponent(Request.EntityID, Request.Type);
        }
        
        while (!EntityDestroyRequests.empty())
        {
            entt::entity Entity = EntityDestroyRequests.back();
            EntityDestroyRequests.pop();

            if (Entity == SelectedEntity)
            {
                if (CopiedEntity == SelectedEntity)
                {
                    CopiedEntity = entt::null;
                }
                
                SetSelectedEntity(entt::null);
            }
            
            World->DestroyEntity(Entity);
            OutlinerListView.MarkTreeDirty();
        }

        if (World->GetEntityRegistry().valid(SelectedEntity))
        {
            World->GetEntityRegistry().emplace_or_replace<FNeedsTransformUpdate>(SelectedEntity);
            
            if (bViewportHovered)
            {
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_C))
                {
                    CopiedEntity = SelectedEntity;
                }

                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D))
                {
                    entt::entity New = entt::null;
                    CopyEntity(New, SelectedEntity);
                }
            }
        }

        if (World->GetEntityRegistry().valid(CopiedEntity) && bViewportHovered)
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_V))
            {
                entt::entity New = entt::null;
                CopyEntity(New, CopiedEntity);
            }
        }

        if (World->GetEntityRegistry().valid(SelectedEntity) && bViewportHovered)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                EntityDestroyRequests.push(SelectedEntity);
            }
        }
    }

    void FWorldEditorTool::EndFrame()
    {
        using namespace entt::literals;
        
        if (!World->GetEntityRegistry().valid(SelectedEntity))
        {
            return;
        }
        
        CComponentVisualizerRegistry& ComponentVisualizerRegistry = CComponentVisualizerRegistry::Get();
        
        ECS::Utils::ForEachComponent([&](void*, entt::basic_sparse_set<>& Set, const entt::meta_type& Type)
        {
            if (entt::meta_any ReturnValue = ECS::InvokeMetaFunc(Type, "static_struct"_hs))
            {
                CStruct* StructType = ReturnValue.cast<CStruct*>();

                if (CComponentVisualizer* Visualizer = ComponentVisualizerRegistry.GetComponentVisualizer(StructType))
                {
                    Visualizer->Draw(World, World->GetEntityRegistry(), SelectedEntity);
                }
                
            }
        }, World->GetEntityRegistry(), SelectedEntity);
    }

    void FWorldEditorTool::DrawToolMenu(const FUpdateContext& UpdateContext)
    {
        
    }

    void FWorldEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGuiID dockLeft = 0;
        ImGuiID dockRight = 0;

        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Right, 0.25f, &dockRight, &dockLeft);

        ImGuiID dockRightTop = 0;
        ImGuiID dockRightBottom = 0;

        ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.25f, &dockRightTop, &dockRightBottom);

        ImGuiID dockRightBottomLeft = 0;
        ImGuiID dockRightBottomRight = 0;

        ImGui::DockBuilderSplitNode(dockRightBottom, ImGuiDir_Right, 0.5f, &dockRightBottomRight, &dockRightBottomLeft);

        ImGui::DockBuilderDockWindow(GetToolWindowName(ViewportWindowName).c_str(), dockLeft);
        ImGui::DockBuilderDockWindow(GetToolWindowName("Outliner").c_str(), dockRightTop);
        ImGui::DockBuilderDockWindow(GetToolWindowName("Details").c_str(), dockRightBottomLeft);
        ImGui::DockBuilderDockWindow(GetToolWindowName(SystemOutlinerName).c_str(), dockRightBottomRight);
        ImGui::DockBuilderDockWindow(GetToolWindowName(WorldSettingsName).c_str(), dockRightBottom);

    }

    void FWorldEditorTool::DrawViewportOverlayElements(const FUpdateContext& UpdateContext, ImTextureRef ViewportTexture, ImVec2 ViewportSize)
    {
        if (bViewportHovered)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Space))
            {
                CycleGuizmoOp();
            }
        }
        
        SCameraComponent& CameraComponent = World->GetEntityRegistry().get<SCameraComponent>(EditorEntity);
    
        glm::mat4 ViewMatrix = CameraComponent.GetViewMatrix();
        glm::mat4 ProjectionMatrix = CameraComponent.GetProjectionMatrix();
        ProjectionMatrix[1][1] *= -1.0f;
        
        ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ViewportSize.x, ViewportSize.y);

        if (World->GetEntityRegistry().valid(SelectedEntity))
        {
            STransformComponent& SelectedTransformComponent = World->GetEntityRegistry().get<STransformComponent>(SelectedEntity);
            if (CameraComponent.GetViewVolume().GetFrustum().IsInside(SelectedTransformComponent.WorldTransform.Location))
            {
                glm::mat4 EntityMatrix = SelectedTransformComponent.GetMatrix();

                float* SnapValues = nullptr;
                float SnapArray[3] = { 0.0f, 0.0f, 0.0f };

                if (bGuizmoSnapEnabled)
                {
                    switch (GuizmoOp)
                    {
                    case ImGuizmo::TRANSLATE:
                        SnapArray[0] = GuizmoSnapTranslate;
                        SnapArray[1] = GuizmoSnapTranslate;
                        SnapArray[2] = GuizmoSnapTranslate;
                        SnapValues = SnapArray;
                        break;

                    case ImGuizmo::ROTATE:
                        SnapArray[0] = GuizmoSnapRotate;
                        SnapArray[1] = GuizmoSnapRotate;
                        SnapArray[2] = GuizmoSnapRotate;
                        SnapValues = SnapArray;
                        break;

                    case ImGuizmo::SCALE:
                        SnapArray[0] = GuizmoSnapScale;
                        SnapArray[1] = GuizmoSnapScale;
                        SnapArray[2] = GuizmoSnapScale;
                        SnapValues = SnapArray;
                        break;
                    }
                }

                ImGuizmo::Manipulate(glm::value_ptr(ViewMatrix), glm::value_ptr(ProjectionMatrix), GuizmoOp, GuizmoMode, glm::value_ptr(EntityMatrix), nullptr, SnapValues);
            
                if (ImGuizmo::IsUsing())
                {
                    bImGuizmoUsedOnce = true;
                
                    glm::mat4 Matrix = EntityMatrix;
                    glm::vec3 Translation, Scale, Skew;
                    glm::quat Rotation;
                    glm::vec4 Perspective;

                    if (FRelationshipComponent* RelationshipComponent = World->GetEntityRegistry().try_get<FRelationshipComponent>(SelectedEntity))
                    {
                        if (RelationshipComponent->Parent != entt::null)
                        {
                            STransformComponent& ParentTransform = World->GetEntityRegistry().get<STransformComponent>(RelationshipComponent->Parent);
                            glm::mat4 ParentWorldMatrix = ParentTransform.WorldTransform.GetMatrix();
                            glm::mat4 ParentWorldInverse = glm::inverse(ParentWorldMatrix);
                            glm::mat4 LocalMatrix = ParentWorldInverse * EntityMatrix;
                            Matrix = LocalMatrix;
                        }
                    }
                
                    glm::decompose(Matrix, Scale, Rotation, Translation, Skew, Perspective);
        
                    SelectedTransformComponent.SetLocation(Translation);
                    SelectedTransformComponent.SetScale(Scale);
                    SelectedTransformComponent.SetRotation(Rotation);
                }
            }
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        {
            uint32 PickerWidth = World->GetRenderer()->GetRenderTarget()->GetExtent().x;
            uint32 PickerHeight = World->GetRenderer()->GetRenderTarget()->GetExtent().y;
            
            ImVec2 viewportScreenPos = ImGui::GetWindowPos();
            ImVec2 mousePos = ImGui::GetMousePos();

            ImVec2 MousePosInViewport;
            MousePosInViewport.x = mousePos.x - viewportScreenPos.x;
            MousePosInViewport.y = mousePos.y - viewportScreenPos.y;

            MousePosInViewport.x = glm::clamp(MousePosInViewport.x, 0.0f, ViewportSize.x - 1.0f);
            MousePosInViewport.y = glm::clamp(MousePosInViewport.y, 0.0f, ViewportSize.y - 1.0f);

            float ScaleX = static_cast<float>(PickerWidth) / ViewportSize.x;
            float ScaleY = static_cast<float>(PickerHeight) / ViewportSize.y;

            uint32 TexX = static_cast<uint32>(MousePosInViewport.x * ScaleX);
            uint32 TexY = static_cast<uint32>(MousePosInViewport.y * ScaleY);
            
            bool bOverImGuizmo = bImGuizmoUsedOnce ? ImGuizmo::IsOver() : false;
            
            if ((!bOverImGuizmo) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                entt::entity EntityHandle = World->GetRenderer()->GetEntityAtPixel(TexX, TexY);
                SetSelectedEntity(EntityHandle);
            }
            else if ((!bOverImGuizmo) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                ImVec2 DragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                float DragDistance = sqrtf(DragDelta.x * DragDelta.x + DragDelta.y * DragDelta.y);
            
                if (DragDistance < 5.0f)
                {
                    entt::entity EntityHandle = World->GetRenderer()->GetEntityAtPixel(TexX, TexY);
                    SetSelectedEntity(EntityHandle);
            
                    if (EntityHandle != entt::null)
                    {
                        ImGui::OpenPopup("EntityContextMenu");
                    }
                }
            
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }
        }
        
        if (ImGui::BeginPopup("EntityContextMenu"))
        {
            if (SelectedEntity != entt::null)
            {
                entt::registry& Registry = World->GetEntityRegistry();
                
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::TextUnformatted("ENTITY");
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("%u", (uint32)SelectedEntity);
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                if (ImGui::MenuItem("Delete Entity", "Del"))
                {
                    Registry.destroy(SelectedEntity);
                    SetSelectedEntity(entt::null);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                if (ImGui::BeginMenu("Add Component"))
                {
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Remove Component"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.4f, 1.0f));
                    
                    bool bHasAnyComponents = false;
                    
                    if (!bHasAnyComponents)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::TextUnformatted("No removable components");
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    ImGui::EndMenu();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {
                    entt::entity To;
                    CopyEntity(To, SelectedEntity);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.6f, 1.0f));
                if (ImGui::MenuItem("Copy", "Ctrl+C"))
                {
                    entt::entity To;
                    CopyEntity(To, SelectedEntity);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                ImGui::PopStyleVar(3);
            }
            
            ImGui::EndPopup();
        }
    }

    void FWorldEditorTool::DrawViewportToolbar(const FUpdateContext& UpdateContext)
    {
        if (!IsAssetEditorTool() && !bSimulatingWorld)
        {
            return;
        }
        
        constexpr float Padding = 8.0f;
        constexpr float ItemSpacing = 6.0f;
        constexpr float ButtonSize = 32.0f;
        constexpr float CornerRounding = 8.0f;
        
        ImVec2 Pos = ImGui::GetWindowPos();
        ImGui::SetNextWindowPos(Pos + ImVec2(Padding, Padding));
        ImGui::SetNextWindowBgAlpha(0.85f);
    
        ImGuiWindowFlags WindowFlags = 
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_AlwaysAutoResize;
    
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Padding, Padding));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, CornerRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ItemSpacing, ItemSpacing));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    
        if (ImGui::Begin("##ViewportToolbar", nullptr, WindowFlags))
        {
            ImGui::BeginGroup();
        
            DrawSimulationControls(ButtonSize);
        
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
        
            DrawCameraControls(ButtonSize);
        
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
        
            DrawViewportOptions(ButtonSize);
        
            ImGui::EndGroup();
        }
        ImGui::End();
    
        ImGui::PopStyleVar(4);
    }

    void FWorldEditorTool::PushAddTagModal(entt::entity Entity)
    {
        struct FTagModalState
        {
            char TagBuffer[256] = {0};
            bool bTagExists = false;
        };
        
        TUniquePtr<FTagModalState> State = MakeUniquePtr<FTagModalState>();
        
        ToolContext->PushModal("Add Tag", ImVec2(400.0f, 180.0f), [this, Entity, State = Move(State)](const FUpdateContext& Context) -> bool
        {
            bool bTagAdded = false;
    
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            ImGui::TextUnformatted("Enter a tag name for this entity");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
    
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.16f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.2f, 0.2f, 0.21f, 1.0f));
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            
            bool bInputEnter = ImGui::InputTextWithHint(
                "##TagInput",
                LE_ICON_TAG " Tag name...",
                State->TagBuffer,
                sizeof(State->TagBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue
            );
            
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere(-1);
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
            
            FString TagName(State->TagBuffer);
            State->bTagExists = !TagName.empty() && ECS::Utils::EntityHasTag(TagName, World->GetEntityRegistry(), Entity);
            
            if (State->bTagExists)
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
                ImGui::TextUnformatted(LE_ICON_ALERT_CIRCLE " Tag already exists on this entity");
                ImGui::PopStyleColor();
            }
    
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
    
            constexpr float buttonWidth = 100.0f;
            float const buttonSpacing = ImGui::GetStyle().ItemSpacing.x;
            float const totalWidth = buttonWidth * 2 + buttonSpacing;
            float const availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((availWidth - totalWidth) * 0.5f);
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            bool const bCanAdd = !TagName.empty() && !State->bTagExists;
            
            if (!bCanAdd)
            {
                ImGui::BeginDisabled();
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.65f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));
            
            if (ImGui::Button("Add", ImVec2(buttonWidth, 0)) || (bInputEnter && bCanAdd))
            {
                entt::hashed_string IDType = entt::hashed_string(TagName.c_str());
                auto& Storage = World->GetEntityRegistry().storage<STagComponent>(IDType);
                Storage.emplace(Entity).Tag = TagName;
                bTagAdded = true;
            }
            
            ImGui::PopStyleColor(3);
            
            if (!bCanAdd)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.32f, 1.0f));
            
            bool bShouldClose = false;
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
            {
                bShouldClose = true;
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
            
            return bTagAdded || bShouldClose;
        });
    }

    void FWorldEditorTool::PushAddComponentModal(entt::entity Entity)
    {
        TUniquePtr<ImGuiTextFilter> Filter = MakeUniquePtr<ImGuiTextFilter>();
        ToolContext->PushModal("Add Component", ImVec2(650.0f, 500.0f), [this, Entity, Filter = Move(Filter)](const FUpdateContext& Context) -> bool
        {
            bool bComponentAdded = false;
    
            // Modal header styling
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            ImGui::TextUnformatted("Select a component to add to the entity");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
    
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.16f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.2f, 0.2f, 0.21f, 1.0f));
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            Filter->Draw(LE_ICON_BRIEFCASE_SEARCH " Search Components...", ImGui::GetContentRegionAvail().x);
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
            
            ImGui::Spacing();
    
            // Component list area
            float const tableHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 2;
            
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12, 8));
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.14f, 0.14f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.16f, 0.16f, 0.17f, 1.0f));
            
            if (ImGui::BeginTable("##ComponentsList", 2, 
                ImGuiTableFlags_NoSavedSettings | 
                ImGuiTableFlags_Borders | 
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY, 
                ImVec2(0, tableHeight)))
            {
                ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();
    
                ImGui::PushID((int)Entity);
    
                struct ComponentInfo
                {
                    const char* Name;
                    const char* Category;
                    entt::meta_type MetaType;
                };
                
                TVector<ComponentInfo> Components;
                
                for(auto&& [_, MetaType]: entt::resolve())
                {
                    using namespace entt::literals;
                    entt::meta_any Any = ECS::InvokeMetaFunc(MetaType, "static_struct"_hs);
                    CStruct* Type = Any.cast<CStruct*>();
                    LUM_ASSERT(Type)
                    
                    const char* ComponentName = Type->GetName().c_str();
                    
                    if (!Filter->PassFilter(ComponentName))
                    {
                        continue;
                    }
                    
                    // Categorize component (you can enhance this logic)
                    const char* Category = "General";
                    Components.push_back({ComponentName, Category, MetaType});
                }
                
                eastl::sort(Components.begin(), Components.end(), [](const ComponentInfo& a, const ComponentInfo& b)
                {
                    int categoryCompare = strcmp(a.Category, b.Category);
                    if (categoryCompare != 0)
                    {
                        return categoryCompare < 0;
                    }
                    return strcmp(a.Name, b.Name) < 0;
                });
                
                // Draw components with category colors
                for (const ComponentInfo& CompInfo : Components)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    
                    // Color code by category
                    ImVec4 IconColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                    const char* Icon = LE_ICON_CUBE;
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, IconColor);
                    ImGui::TextUnformatted(Icon);
                    ImGui::PopStyleColor();
                    
                    ImGui::SameLine();
                    
                    // Component name as selectable
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.5f, 0.8f, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.6f, 0.9f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.35f, 0.65f, 0.95f, 0.6f));
                    
                    if (ImGui::Selectable(CompInfo.Name, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        using namespace entt::literals;

                        ECS::InvokeMetaFunc(CompInfo.MetaType, "emplace"_hs, entt::forward_as_meta(World->GetEntityRegistry()), SelectedEntity, entt::forward_as_meta(entt::meta_any{}));
                        bComponentAdded = true;
                    }
                    
                    ImGui::PopStyleColor(3);
                    
                    // Category column
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::TextUnformatted(CompInfo.Category);
                    ImGui::PopStyleColor();
                }
                
                ImGui::PopID();
                ImGui::EndTable();
            }
            
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
    
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
    
            // Bottom buttons
            float buttonWidth = 120.0f;
            float availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((availWidth - buttonWidth) * 0.5f);
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.32f, 1.0f));
            
            bool shouldClose = false;
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
            {
                shouldClose = true;
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
            
            if (bComponentAdded)
            {
                RebuildPropertyTables(Entity);
            }
            
            return bComponentAdded || shouldClose;
        });
    }

    void FWorldEditorTool::PushRenameEntityModal(entt::entity Entity)
    {
        ToolContext->PushModal("Rename Entity", ImVec2(600.0f, 350.0f), [this, Entity](const FUpdateContext& Context) -> bool
        {
            FName& Name = World->GetEntityRegistry().get<SNameComponent>(Entity).Name;
            FString CopyName = Name.ToString();
            
            if (ImGui::InputText("##Name", const_cast<char*>(CopyName.c_str()), 256, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                Name = CopyName.c_str();
                return true;
            }
            
            if (ImGui::Button("Cancel"))
            {
                return true;
            }

            return false;
        });
    }

    void FWorldEditorTool::OnSave()
    {
		if (!IsAssetEditorTool())
        {
            ImGuiX::Notifications::NotifyError("Cannot save world: No associated package.");
            return;
        }

        FString FullPath = Paths::ResolveVirtualPath(World->GetPackage()->GetName().ToString());
        Paths::AddPackageExtension(FullPath);

        if (CPackage::SavePackage(World->GetPackage(), FullPath.c_str()))
        {
            FAssetRegistry::Get().AssetSaved(World);
            ImGuiX::Notifications::NotifySuccess("Successfully saved world: \"{0}\"", World->GetName().c_str());
        }
        else
        {
            ImGuiX::Notifications::NotifyError("Failed to save world: \"{0}\"", World->GetName().c_str());
        }
    }

    bool FWorldEditorTool::IsAssetEditorTool() const
    {
        return World->GetPackage() != nullptr;
    }

    void FWorldEditorTool::NotifyPlayInEditorStart()
    {
        bGamePreviewRunning = true;
    }

    void FWorldEditorTool::NotifyPlayInEditorStop()
    {
         bGamePreviewRunning = false;
    }

    void FWorldEditorTool::SetWorld(CWorld* InWorld)
    {
        SetSelectedEntity(entt::null);
        
        FEditorTool::SetWorld(InWorld);
        
        OutlinerListView.MarkTreeDirty();
    }

    void FWorldEditorTool::OnEntityDestroyed(entt::registry& Registry, entt::entity Entity)
    {
        if (SelectedEntity == Entity)
        {
            SetSelectedEntity(entt::null);
        }
    }

    void FWorldEditorTool::DrawSimulationControls(float ButtonSize)
    {
        const ImVec4 ActiveColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        const ImVec4 InactiveColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        const ImVec2 BtnSize = ImVec2(ButtonSize, ButtonSize);
        
        if (!bGamePreviewRunning)
        {
            if (!bSimulatingWorld)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
                if (ImGuiX::IconButton(LE_ICON_PLAY, "##PlayBtn", 0xFFFFFFFF, BtnSize))
                {
                    OnGamePreviewStartRequested.Broadcast();
                }
                ImGui::PopStyleColor(2);
                
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                {
                    ImGui::SetTooltip("Play (Start game preview)");
                }
                
                ImGui::SameLine();
                
                if (ImGuiX::IconButton(LE_ICON_COG_BOX, "##SimulateBtn", 0xFFFFFFFF, BtnSize))
                {
                    SetWorldNewSimulate(true);
                }
                
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                {
                    ImGui::SetTooltip("Simulate (Run physics without gameplay)");
                }
            }
            else
            {
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    SetWorldNewSimulate(false);
                }
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.1f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                if (ImGuiX::IconButton(LE_ICON_COG_BOX, "##SimulateActiveBtn", 0xFFFFFFFF, BtnSize))
                {
                    SetWorldNewSimulate(false);
                }
                ImGui::PopStyleColor(2);
                
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                {
                    ImGui::SetTooltip("Stop Simulation (ESC)");
                }
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            if (ImGuiX::IconButton(LE_ICON_STOP, "##StopBtn", 0xFFFFFFFF, BtnSize))
            {
                OnGamePreviewStopRequested.Broadcast();
            }
            ImGui::PopStyleColor(2);
            
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("Stop Game Preview");
            }
        }
    }

    void FWorldEditorTool::DrawCameraControls(float ButtonSize)
    {
        const ImVec2 BtnSize = ImVec2(ButtonSize, ButtonSize);
        float Speed = World->GetEntityRegistry().get<SVelocityComponent>(EditorEntity).Speed;

        if (ImGuiX::IconButton(LE_ICON_SPADE, "##CameraSpeed", 0xFFFFFFFF, BtnSize))
        {
            ImGui::OpenPopup("CameraSpeedPopup");
        }
    
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Camera Speed: %.1fx", Speed);
        }
    
        
        if (ImGui::BeginPopup("CameraSpeedPopup", ImGuiWindowFlags_NoMove))
        {
            ImGui::Text("Camera Speed");
            ImGui::Separator();
            
            if (ImGui::SliderFloat("##Speed", &Speed, 0.1f, 100.0f, "%.1fx"))
            {
                World->GetEntityRegistry().get<SVelocityComponent>(EditorEntity).Speed = Speed;
            }
        
            if (ImGui::Button("Reset to 1.0x", ImVec2(-1, 0)))
            {
                World->GetEntityRegistry().get<SVelocityComponent>(EditorEntity).Speed = 1.0f;
                ImGui::CloseCurrentPopup();
            }
        
            ImGui::EndPopup();
        }
    
        ImGui::SameLine();
    
        if (ImGuiX::IconButton(LE_ICON_CROSSHAIRS, "##FocusSelection", 0xFFFFFFFF, BtnSize))
        {
            //FocusOnSelection();
        }
    
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Focus on Selection (F)");
        }
    }

    void FWorldEditorTool::DrawViewportOptions(float ButtonSize)
    {
        const ImVec2 BtnSize = ImVec2(ButtonSize, ButtonSize);
    
        bool GridActive = false;
        if (GridActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 0.6f));
        }
        
        if (ImGuiX::IconButton(LE_ICON_GRID, "##GridToggle", 0xFFFFFFFF, BtnSize))
        {
        }
        
        if (GridActive)
        {
            ImGui::PopStyleColor();
        }
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Toggle Grid (G)");
        }
        
        ImGui::SameLine();
        
        const char* Icon = nullptr;
        switch (GuizmoOp)
        {
        case ImGuizmo::OPERATION::TRANSLATE:
            {
                Icon = LE_ICON_AXIS_ARROW;
            }
            break;
        case ImGuizmo::OPERATION::ROTATE:
            {
                Icon = LE_ICON_ROTATE_360;
            }
            break;
        case ImGuizmo::OPERATION::SCALE:
            {
                Icon = LE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT;
            }
            break;
        }
        
        if (ImGuiX::IconButton(Icon, "##GizmoMode", 0xFFFFFFFF, BtnSize))
        {
            CycleGuizmoOp();
        }
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Gizmo: %s (R)", ImGuiX::ImGuizmoOpToString(GuizmoOp).data());
        }
        
        if (bGuizmoSnapEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 0.6f));
        }
        
        ImGui::SameLine();
    
        bool bSnapWasEnabled = bGuizmoSnapEnabled;
        if (ImGuiX::IconButton(LE_ICON_MAGNET, "##SnapToggle", 0xFFFFFFFF, BtnSize))
        {
            bGuizmoSnapEnabled = !bGuizmoSnapEnabled;
        }
    
        if (bSnapWasEnabled)
        {
            ImGui::PopStyleColor();
        }
    
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Snap Settings (Click to toggle) (Right click for config)");
        }
    
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
        {
            ImGui::OpenPopup("SnapSettingsPopup");
        }
    
        if (ImGui::BeginPopup("SnapSettingsPopup", ImGuiWindowFlags_NoMove))
        {
            DrawSnapSettingsPopup();
            ImGui::EndPopup();
        }
    
        ImGui::SameLine();
        
        if (ImGuiX::IconButton(LE_ICON_EYE, "##ViewMode", 0xFFFFFFFF, BtnSize))
        {
            ImGui::OpenPopup("ViewModePopup");
        }
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("View Mode Options");
        }
        
        if (ImGui::BeginPopup("ViewModePopup", ImGuiWindowFlags_NoMove))
        {
            ImGui::Text("Rendering Mode");
            ImGui::Separator();
            
            if (ImGui::Selectable("Lit"))
            {
            }
            if (ImGui::Selectable("Unlit"))
            {
            }
            if (ImGui::Selectable("Wireframe"))
            {
            }
            
            ImGui::Separator();
            //ImGui::Checkbox("Show Physics", &bShowPhysics);
            //ImGui::Checkbox("Show Collision", &bShowCollision);
            //ImGui::Checkbox("Show Bounds", &bShowBounds);
            
            ImGui::EndPopup();
        }
    }
    
    void FWorldEditorTool::DrawSnapSettingsPopup()
    {
        ImGui::Text("Snap Settings");
        ImGui::Separator();
        
        if (ImGui::Checkbox("Enable Snap", &bGuizmoSnapEnabled))
        {
            
        }
        
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.3f));
        if (ImGui::CollapsingHeader("Translation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("Translate");
            ImGui::Indent();
            
            ImGui::BeginDisabled(!bGuizmoSnapEnabled);
            
            ImGui::Text("Presets:");
            ImGui::SameLine();
            
            if (ImGui::Button("0.1"))
            {
                GuizmoSnapTranslate = 0.1f;
            }
            ImGui::SameLine();
            if (ImGui::Button("1.0"))
            {
                GuizmoSnapTranslate = 1.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("5.0"))
            {
                GuizmoSnapTranslate = 5.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("10"))
            {
                GuizmoSnapTranslate = 10.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("50"))
            {
                GuizmoSnapTranslate = 50.0f;
            }
            
            ImGui::DragFloat("Value##Translation", &GuizmoSnapTranslate, 0.1f, 0.01f, 1000.0f, "%.2f units");
            
            ImGui::EndDisabled();
            ImGui::Unindent();
            ImGui::PopID();
        }
        
        if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("Rotate");
            ImGui::Indent();
            
            ImGui::BeginDisabled(!bGuizmoSnapEnabled);
            
            ImGui::Text("Presets:");
            ImGui::SameLine();
            
            if (ImGui::Button("1 " LE_ICON_ANGLE_ACUTE))
            {
                GuizmoSnapRotate = 1.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("5 " LE_ICON_ANGLE_ACUTE))
            {
                GuizmoSnapRotate = 5.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("15 " LE_ICON_ANGLE_ACUTE))
            {
                GuizmoSnapRotate = 15.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("45 " LE_ICON_ANGLE_ACUTE))
            {
                GuizmoSnapRotate = 45.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("90 " LE_ICON_ANGLE_ACUTE))
            {
                GuizmoSnapRotate = 90.0f;
            }
            
            ImGui::DragFloat("Value##Rotation", &GuizmoSnapRotate, 0.5f, 0.1f, 180.0f, "%.1f " LE_ICON_ANGLE_ACUTE);
            
            ImGui::EndDisabled();
            ImGui::Unindent();
            ImGui::PopID();
        }
        
        if (ImGui::CollapsingHeader("Scale", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("Scale");
            ImGui::Indent();
            
            ImGui::BeginDisabled(!bGuizmoSnapEnabled);
            
            ImGui::Text("Presets:");
            ImGui::SameLine();
            
            if (ImGui::Button("0.1"))
            {
                GuizmoSnapScale = 0.1f;
            }
            ImGui::SameLine();
            if (ImGui::Button("0.25"))
            {
                GuizmoSnapScale = 0.25f;
            }
            ImGui::SameLine();
            if (ImGui::Button("0.5"))
            {
                GuizmoSnapScale = 0.5f;
            }
            ImGui::SameLine();
            if (ImGui::Button("1.0"))
            {
                GuizmoSnapScale = 1.0f;
            }
            
            ImGui::DragFloat("Value##Scale", &GuizmoSnapScale, 0.01f, 0.01f, 10.0f, "%.2f");
            
            ImGui::EndDisabled();
            ImGui::Unindent();
            ImGui::PopID();
        }
        
        ImGui::PopStyleColor();
    }

    void FWorldEditorTool::SetWorldNewSimulate(bool bShouldSimulate)
    {
        if (bShouldSimulate != bSimulatingWorld && bShouldSimulate == true)
        {
            bSimulatingWorld = true;
            
            STransformComponent TransformCopy = World->GetEntityRegistry().get<STransformComponent>(EditorEntity);
            SCameraComponent CameraCopy =  World->GetEntityRegistry().get<SCameraComponent>(EditorEntity);

            World->SetActive(false);
            WorldState = World;
            World = CWorld::DuplicateWorld(WorldState);
            World->SetSimulating(true);

            entt::entity PreviousSelectedEntity = SelectedEntity;
            SetSelectedEntity(entt::null);
            
            OutlinerListView.MarkTreeDirty();
            
            SetupWorldForTool();

            SetSelectedEntity(PreviousSelectedEntity);

            
            World->GetEntityRegistry().patch<STransformComponent>(EditorEntity, [TransformCopy](STransformComponent& Patch)
            {
                Patch = TransformCopy;
            });

            World->GetEntityRegistry().patch<SCameraComponent>(EditorEntity, [CameraCopy](SCameraComponent& Patch)
            {
                Patch = CameraCopy;
            });
        }
        else if (bShouldSimulate != bSimulatingWorld && bShouldSimulate == false)
        {
            World->SetSimulating(false);
            bSimulatingWorld = false;

            STransformComponent TransformCopy = World->GetEntityRegistry().get<STransformComponent>(EditorEntity);
            SCameraComponent CameraCopy =  World->GetEntityRegistry().get<SCameraComponent>(EditorEntity);

            entt::entity PreviousSelectedEntity = SelectedEntity;

            SetWorld(WorldState);
            WorldState->SetActive(true);
            
            SetSelectedEntity(PreviousSelectedEntity);

            
            World->GetEntityRegistry().patch<STransformComponent>(EditorEntity, [TransformCopy](STransformComponent& Patch)
            {
                Patch = TransformCopy;
            });

            World->GetEntityRegistry().patch<SCameraComponent>(EditorEntity, [CameraCopy](SCameraComponent& Patch)
            {
                Patch = CameraCopy;
            });
            
            WorldState = nullptr;
        }
    }

    void FWorldEditorTool::DrawCreateEntityMenu()
    {
        ImGui::SetNextWindowSize(ImVec2(400.0f, 500.0f), ImGuiCond_Always);
    
        if (ImGui::BeginPopup("CreateEntityMenu", ImGuiWindowFlags_NoMove))
        {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), LE_ICON_PLUS " Create New Entity");
            ImGui::PopFont();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::SetNextItemWidth(-1);
            
            AddEntityComponentFilter.Draw(LE_ICON_FOLDER_SEARCH " Search templates...");
            
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            
            ImGui::Spacing();
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
            
            if (ImGui::BeginChild("TemplateList", ImVec2(0, -35.0f), true))
            {
                for(auto &&[_, MetaType]: entt::resolve())
                {
                    using namespace entt::literals;
                    
                    entt::meta_any Any = ECS::InvokeMetaFunc(MetaType, "static_struct"_hs);
                    CStruct* Struct = Any.cast<CStruct*>();
                    LUM_ASSERT(Struct)
                    
                    if (Struct == STransformComponent::StaticStruct() || Struct == SNameComponent::StaticStruct() || Struct == STagComponent::StaticStruct())
                    {
                        continue;
                    }

                    
                    ImGui::PushID(Struct);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.21f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.35f, 0.45f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
                    
                    const float ButtonWidth = ImGui::GetContentRegionAvail().x;
                    
                    if (ImGui::Button(Struct->GetName().c_str(), ImVec2(ButtonWidth, 0.0f)))
                    {
                        CreateEntityWithComponent(Struct);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    ImGui::PopStyleVar(2);
                    ImGui::PopStyleColor(3);
                    
                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }
            ImGui::EndChild();
            
            ImGui::PopStyleVar(2);
            
            ImGui::Separator();
            
            ImGui::BeginGroup();
            {
                if (ImGui::Button("Cancel", ImVec2(80.0f, 0.0f)))
                {
                    ImGui::CloseCurrentPopup();
                    AddEntityComponentFilter.Clear();
                }
                
                ImGui::SameLine();
                
                // Quick empty entity button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
                if (ImGui::Button(LE_ICON_CUBE " Empty Entity", ImVec2(-1, 0.0f)))
                {
                    CreateEntity();
                    ImGui::CloseCurrentPopup();
                    AddEntityComponentFilter.Clear();
                }
                ImGui::PopStyleColor();
                
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Create entity without any components");
                }
            }
            ImGui::EndGroup();
            
            ImGui::EndPopup();
        }
    }

    void FWorldEditorTool::DrawFilterOptions()
    {
        ImGui::Text(LE_ICON_CUBE " Filter by Component");
        ImGui::Separator();
        
    }

    void FWorldEditorTool::SetSelectedEntity(entt::entity Entity)
    {
        if (Entity == SelectedEntity)
        {
            return;
        }

        SelectedEntity = Entity;
        
        OutlinerListView.MarkTreeDirty();
        RebuildPropertyTables(Entity);
        World->SetSelectedEntity(SelectedEntity);
    }

    void FWorldEditorTool::RebuildSceneOutliner(FTreeListView* View)
    {
        TFunction<void(entt::entity, FEntityListViewItem*)> AddEntityRecursive;
        
        AddEntityRecursive = [&](entt::entity Entity, FEntityListViewItem* ParentItem)
        {
            FEntityListViewItem* Item = nullptr;
            if (ParentItem)
            {
                Item = ParentItem->AddChild<FEntityListViewItem>(ParentItem, World->GetEntityRegistry(), Entity);
            }
            else
            {
                Item = OutlinerListView.AddItemToTree<FEntityListViewItem>(ParentItem, World->GetEntityRegistry(), Entity);
            }

            if (Item->GetEntity() == SelectedEntity)
            {
                OutlinerListView.SetSelection(Item, OutlinerContext);
            }

            if (FRelationshipComponent* Rel = World->GetEntityRegistry().try_get<FRelationshipComponent>(Item->GetEntity()))
            {
                for (SIZE_T i = 0; i < Rel->NumChildren; ++i)
                {
                    AddEntityRecursive(Rel->Children[i], Item);
                }
            }
        };
        

        for (entt::entity Entity : World->GetEntityRegistry().view<entt::entity>(entt::exclude<FHideInSceneOutliner>))
        {
            if (FRelationshipComponent* Rel = World->GetEntityRegistry().try_get<FRelationshipComponent>(Entity))
            {
                if (Rel->Parent != entt::null)
                {
                    continue;
                }
            }

            AddEntityRecursive(Entity, nullptr);
        }
    }


    void FWorldEditorTool::HandleEntityEditorDragDrop(FTreeListViewItem* DropItem)
    {
        const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(FEntityListViewItem::DragDropID, ImGuiDragDropFlags_AcceptBeforeDelivery);
        if (Payload && Payload->IsDelivery())
        {
            uintptr_t* RawPtr = static_cast<uintptr_t*>(Payload->Data);
            auto* SourceItem = reinterpret_cast<FEntityListViewItem*>(*RawPtr);  // NOLINT(performance-no-int-to-ptr)
            auto* DestinationItem = static_cast<FEntityListViewItem*>(DropItem);

            if (SourceItem == DestinationItem)
            {
                return;
            }

            World->ReparentEntity(SourceItem->GetEntity(), DestinationItem->GetEntity());
            
            OutlinerListView.MarkTreeDirty();
        }
    }

    void FWorldEditorTool::DrawWorldSettings(bool bFocused)
    {
        DrawEntityEditor(bFocused, World->GetSingletonEntity());
    }

    void FWorldEditorTool::DrawOutliner(bool bFocused)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();
        
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
            
            constexpr float ButtonWidth = 30.0f;
            if (ImGui::Button(LE_ICON_PLUS, ImVec2(ButtonWidth, 0.0f)))
            {
                ImGui::OpenPopup("CreateEntityMenu");
            }
            
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Add something new to the world.");
            }

            DrawCreateEntityMenu();
            
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ButtonWidth));
            EntityFilterState.FilterName.Draw("##Search");
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            
            if (!EntityFilterState.FilterName.IsActive())
            {
                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                ImVec2 TextPos = ImGui::GetItemRectMin();
                TextPos.x += Style.FramePadding.x + 2.0f;
                TextPos.y += Style.FramePadding.y;
                DrawList->AddText(TextPos, IM_COL32(100, 100, 110, 255), LE_ICON_FILE_SEARCH " Search entities...");
            }
            
            ImGui::SameLine();
            
            const bool bFilterActive = EntityFilterState.FilterName.IsActive();
            ImGui::PushStyleColor(ImGuiCol_Button, 
                bFilterActive ? ImVec4(0.4f, 0.45f, 0.65f, 1.0f) : ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                bFilterActive ? ImVec4(0.5f, 0.55f, 0.75f, 1.0f) : ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            if (ImGui::Button(LE_ICON_FILTER_SETTINGS "##ComponentFilter", ImVec2(ButtonWidth, 0.0f)))
            {
                ImGui::OpenPopup("FilterPopup");
            }
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(bFilterActive ? "Filters active - Click to configure" : "Configure filters");
            }
            
            if (ImGui::BeginPopup("FilterPopup"))
            {
                ImGui::SeparatorText("Component Filters");
                DrawFilterOptions();
                ImGui::EndPopup();
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.1f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
            
            if (ImGui::BeginChild("EntityList", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar))
            {
                OutlinerListView.Draw(OutlinerContext);
            }
            ImGui::EndChild();
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        
    }

    void FWorldEditorTool::DrawSystems(bool bFocused)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();
    
        const float AvailWidth = ImGui::GetContentRegionAvail().x;
        uint32 SystemCount = 0;
        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            SystemCount += (uint32)World->GetSystemsForUpdateStage((EUpdateStage)i).size();
        }
        
        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();
    
            ImGui::BeginHorizontal("##SystemCount");
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), LE_ICON_CUBE " Systems");
            
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, Style.FramePadding.y));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                
                ImGui::Button(FFixedString().sprintf("%u", SystemCount).c_str());
            }
            ImGui::EndHorizontal();
            
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
        }
        ImGui::EndGroup();
        
        ImGui::SameLine(AvailWidth - 80.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        
        if (ImGui::BeginChild("SystemList", ImVec2(0, 0), true))
        {
            // Stage color palette
            constexpr ImVec4 StageColors[] = 
            {
                ImVec4(0.3f, 0.5f, 0.7f, 1.0f),
                ImVec4(0.5f, 0.6f, 0.3f, 1.0f),
                ImVec4(0.7f, 0.4f, 0.3f, 1.0f),
                ImVec4(0.6f, 0.3f, 0.6f, 1.0f),
                ImVec4(0.3f, 0.6f, 0.5f, 1.0f),
                ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
            };
            
            for (int i = 0; i < (int)EUpdateStage::Max; ++i)
            {
                EUpdateStage Stage = (EUpdateStage)i;
                const TVector<CEntitySystem*>& Systems = World->GetSystemsForUpdateStage(Stage);
    
                if (Systems.empty())
                {
                    continue;
                }

                TVector<TPair<uint8, CEntitySystem*>> SortedSystems;
                for (CEntitySystem* System : Systems)
                {
                    const FUpdatePriorityList* List = System->GetRequiredUpdatePriorities();
                    uint8 Priority = List->GetPriorityForStage(Stage);
                    SortedSystems.emplace_back(Priority, System);
                }
                
                eastl::sort(SortedSystems.begin(), SortedSystems.end(), [](const TPair<uint8, CEntitySystem*>& LHS, const TPair<uint8, CEntitySystem*>& RHS)
                {
                    return LHS.first > RHS.first;
                });
    
                ImGui::PushStyleColor(ImGuiCol_Header, StageColors[i]);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(
                    StageColors[i].x * 1.2f, 
                    StageColors[i].y * 1.2f, 
                    StageColors[i].z * 1.2f, 
                    1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                
                bool IsOpen = ImGui::CollapsingHeader(FFixedString().sprintf("%s (%zu)", GUpdateStageNames[i], Systems.size()).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
    
                if (IsOpen)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
                    ImGui::Indent();
    
                    
                    for (size_t Sys = 0; Sys < SortedSystems.size(); ++Sys)
                    {
                        const auto& Pair = SortedSystems[Sys];
                        CEntitySystem* System = Pair.second;
                        
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
                        
                        ImGui::BeginHorizontal((int)Sys);
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopStyleColor(3);
                        
                        ImGui::Spring(0.0f, 8.0f);
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", System->GetName().c_str());
                        
                        ImGui::Spring(1.0f);
                        
                        ImVec4 StateColor = ImVec4(0.3f, 0.7f, 0.3f, 1.0f); 
                        
                        ImGui::TextColored(StateColor, LE_ICON_CHECK);
                        
                        ImGui::EndHorizontal();
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopStyleColor();
                    }
    
                    ImGui::Unindent();
                    ImGui::PopStyleVar();
                    
                    if (i < (int)EUpdateStage::Max - 1)
                    {
                        ImGui::Spacing();
                    }
                }
                else if (i < (int)EUpdateStage::Max - 1)
                {
                    ImGui::Spacing();
                }
            }
        }
        ImGui::EndChild();
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void FWorldEditorTool::DrawEntityProperties(entt::entity Entity)
    {
        if (World->IsSimulating())
        {
            ImGui::BeginDisabled();
        }
        
        SNameComponent* NameComponent = World->GetEntityRegistry().try_get<SNameComponent>(Entity);
        FName EntityName = NameComponent ? NameComponent->Name : eastl::to_string((uint32)Entity);
        
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        constexpr ImGuiTableFlags Flags = 
        ImGuiTableFlags_BordersOuter | 
        ImGuiTableFlags_NoBordersInBodyUntilResize | 
        ImGuiTableFlags_SizingFixedFit;
        
        if (ImGui::BeginTable("##EntityName", 1, Flags))
        {
            ImGui::TableSetupColumn("##Editor", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::BeginHorizontal(EntityName.c_str());
        
            ImGuiX::Font::PushFont(ImGuiX::Font::EFont::LargeBold);
            ImGui::AlignTextToFramePadding();
            ImGuiX::Text("Entity: {}", EntityName);
            ImGui::PopFont();

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(35, 35, 35, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.65f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));
        
            if (ImGui::Button(LE_ICON_PLUS))
            {
                PushAddComponentModal(Entity);
            }
        
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("Add Component");
            }
        
            ImGui::PopStyleColor(3);
        
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.65f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));
        
            if (ImGui::Button(LE_ICON_TAG))
            {
                PushAddTagModal(Entity);
            }
        
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("Add Tag");
            }
        
            ImGui::PopStyleColor(3);
        
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
        
            if (ImGui::Button(LE_ICON_TRASH_CAN))
            {
                if (Dialogs::Confirmation("Confirm Deletion", 
                    "Are you sure you want to delete entity \"{0}\"?\n\nThis action cannot be undone.", 
                    (uint32)Entity))
                {
                    EntityDestroyRequests.push(Entity);
                }
            }
        
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("Delete Entity");
            }
        
            ImGui::PopStyleColor(3);
        
            ImGui::EndHorizontal();
            ImGui::PopStyleVar(3);
            
            ImGui::EndTable();
        }
        
        if (World->IsSimulating())
        {
            ImGui::EndDisabled();
        }
        
        ImGui::SeparatorText("Details");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(LE_ICON_PUZZLE " Tags");
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        DrawTagList(Entity);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(LE_ICON_CUBE " Components");
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        DrawComponentList(Entity);
    }

    void FWorldEditorTool::DrawEntityActionButtons(entt::entity Entity)
    {
        constexpr float ButtonHeight = 32.0f;
        const float AvailWidth = ImGui::GetContentRegionAvail().x;
        const float ButtonWidth = (AvailWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.65f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));

        if (ImGui::Button(LE_ICON_PLUS " Add Component", ImVec2(ButtonWidth, ButtonHeight)))
        {
            PushAddComponentModal(Entity);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Add a new component to this entity");
        }

        ImGui::SameLine();

        if (ImGui::Button(LE_ICON_TAG " Add Tag", ImVec2(ButtonWidth, ButtonHeight)))
        {
            PushAddTagModal(Entity);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Add a runtime tag to this entity to use with runtime views.");
        }

        ImGui::PopStyleColor(3);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));

        if (ImGui::Button(LE_ICON_TRASH_CAN " Destroy", ImVec2(AvailWidth, ButtonHeight)))
        {
            if (Dialogs::Confirmation("Confirm Deletion", "Are you sure you want to delete entity \"{0}\"?\n""\nThis action cannot be undone.", (uint32)Entity))
            {
                EntityDestroyRequests.push(Entity);
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Permanently delete this entity");
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }

    void FWorldEditorTool::DrawComponentList(entt::entity Entity)
    {
        for (TUniquePtr<FPropertyTable>& Table : PropertyTables)
        {
            DrawComponentHeader(Table, Entity);
        
            ImGui::Spacing();
        }
    }

    void FWorldEditorTool::DrawTagList(entt::entity Entity)
    {

        TFixedVector<FName, 4> Tags;
        for (auto [Name, Storage] : World->GetEntityRegistry().storage())
        {
            if (Storage.info() == entt::type_id<STagComponent>())
            {
                if (Storage.contains(Entity))
                {
                    STagComponent* ComponentPtr = static_cast<STagComponent*>(Storage.value(Entity));
                    Tags.push_back(ComponentPtr->Tag);
                }
            }
        }
        
        if (Tags.empty())
        {
            return;
        }
        
        if (World->IsSimulating())
        {
            ImGui::BeginDisabled();
        }
        
        ImGui::PushID("TagList");
        
        // Section header
        ImVec2 CursorPos = ImGui::GetCursorScreenPos();
        ImVec2 HeaderSize = ImVec2(ImGui::GetContentRegionAvail().x, 32.0f);
        
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(CursorPos, ImVec2(CursorPos.x + HeaderSize.x, CursorPos.y + HeaderSize.y), IM_COL32(25, 25, 30, 255), 6.0f);
        
        DrawList->AddRect(CursorPos, ImVec2(CursorPos.x + HeaderSize.x, CursorPos.y + HeaderSize.y), IM_COL32(45, 45, 52, 255), 6.0f, 0, 1.0f);
        
        ImVec2 IconPos = CursorPos;
        IconPos.x += 12.0f;
        IconPos.y += (HeaderSize.y - ImGui::GetTextLineHeight()) * 0.5f;
        DrawList->AddText(IconPos, IM_COL32(150, 170, 200, 255), LE_ICON_TAG);
        
        ImVec2 TitlePos = IconPos;
        TitlePos.x += 24.0f;
        DrawList->AddText(TitlePos, IM_COL32(220, 220, 230, 255), "Tags");
        
        // Tag count badge
        char CountBuf[16];
        snprintf(CountBuf, sizeof(CountBuf), "%zu", Tags.size());
        ImVec2 CountPos = TitlePos;
        CountPos.x += ImGui::CalcTextSize("Tags").x + 8.0f;
        CountPos.y -= 1.0f;
        
        ImVec2 CountBadgeSize = ImGui::CalcTextSize(CountBuf);
        CountBadgeSize.x += 10.0f;
        CountBadgeSize.y += 2.0f;
        
        DrawList->AddRectFilled(CountPos, 
            ImVec2(CountPos.x + CountBadgeSize.x, CountPos.y + CountBadgeSize.y),
            IM_COL32(60, 80, 120, 180), 3.0f);
        DrawList->AddText(ImVec2(CountPos.x + 5.0f, CountPos.y + 1.0f), 
            IM_COL32(180, 200, 240, 255), CountBuf);
        
        ImGui::SetCursorScreenPos(ImVec2(CursorPos.x, CursorPos.y + HeaderSize.y + 4.0f));
        
        // Tag chips
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
        
        float AvailWidth = ImGui::GetContentRegionAvail().x;
        float CurrentX = 0.0f;
        
        FName TagToRemove;
        
        for (const FName& Tag : Tags)
        {
            ImGui::PushID(Tag.c_str());
            
            const char* TagStr = Tag.c_str();
            ImVec2 TagSize = ImGui::CalcTextSize(TagStr);
            float ChipWidth = TagSize.x + 52.0f;
            
            if (CurrentX + ChipWidth > AvailWidth && CurrentX > 0.0f)
            {
                CurrentX = 0.0f;
            }
            else if (CurrentX > 0.0f)
            {
                ImGui::SameLine();
            }
            
            ImVec2 ChipPos = ImGui::GetCursorScreenPos();
            ImVec2 ChipSize = ImVec2(ChipWidth, TagSize.y + 10.0f);
            
            bool bHovered = ImGui::IsMouseHoveringRect(ChipPos, 
                ImVec2(ChipPos.x + ChipSize.x, ChipPos.y + ChipSize.y));
            
            ImU32 ChipBg = bHovered ? IM_COL32(55, 65, 85, 255) : IM_COL32(45, 55, 75, 255);
            ImU32 ChipBorder = bHovered ? IM_COL32(80, 100, 140, 255) : IM_COL32(65, 80, 115, 255);
            
            DrawList->AddRectFilled(ChipPos, ImVec2(ChipPos.x + ChipSize.x, ChipPos.y + ChipSize.y), ChipBg, 12.0f);
            DrawList->AddRect(ChipPos, ImVec2(ChipPos.x + ChipSize.x, ChipPos.y + ChipSize.y), ChipBorder, 12.0f, 0, 1.0f);
            
            DrawList->AddText(ImVec2(ChipPos.x + 10.0f, ChipPos.y + 5.0f), IM_COL32(130, 160, 210, 255), LE_ICON_TAG);
            
            DrawList->AddText(ImVec2(ChipPos.x + 28.0f, ChipPos.y + 5.0f), IM_COL32(200, 210, 230, 255), TagStr);
            
            ImVec2 ClosePos = ImVec2(ChipPos.x + ChipSize.x - 20.0f, ChipPos.y + 5.0f);
            bool bCloseHovered = ImGui::IsMouseHoveringRect(ImVec2(ClosePos.x - 4.0f, ClosePos.y - 4.0f), ImVec2(ClosePos.x + 12.0f, ClosePos.y + 12.0f));
            
            ImU32 CloseColor = bCloseHovered ? IM_COL32(240, 100, 100, 255) : IM_COL32(150, 150, 160, 255);
            DrawList->AddText(ClosePos, CloseColor, LE_ICON_CLOSE);
            
            ImGui::InvisibleButton("##chip", ChipSize);
            
            if (bCloseHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                TagToRemove = Tag;
            }
            
            CurrentX += ChipWidth + ImGui::GetStyle().ItemSpacing.x;
            
            ImGui::PopID();
        }
        
        ImGui::PopStyleVar(3);
        
        if (!TagToRemove.IsNone())
        {
            World->GetEntityRegistry().storage<STagComponent>(entt::hashed_string(TagToRemove.c_str())).remove(Entity);
        }
        
        ImGui::Spacing();
        ImGui::PopID();
        
        if (World->IsSimulating())
        {
            ImGui::EndDisabled();
        }
    }

    void FWorldEditorTool::DrawComponentHeader(const TUniquePtr<FPropertyTable>& Table, entt::entity Entity)
    {
        const char* ComponentName = Table->GetType()->GetName().c_str();
        const bool bIsRequired = (Table->GetType() == STransformComponent::StaticStruct() || 
                                 Table->GetType() == SNameComponent::StaticStruct());
    
        if (Table->GetType() == STagComponent::StaticStruct())
        {
            return;
        }
        
        ImGui::PushID(Table.get());
            
        constexpr ImGuiTableFlags Flags = 
        ImGuiTableFlags_BordersOuter | 
        ImGuiTableFlags_BordersInnerH | 
        ImGuiTableFlags_NoBordersInBodyUntilResize | 
        ImGuiTableFlags_SizingFixedFit;
            
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 8));
        bool bIsOpen = false;
        if (ImGui::BeginTable("GridTable", 1, Flags))
        {
            ImGui::TableSetupColumn("##Header", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            
            
            ImGui::PushStyleColor(ImGuiCol_Header, 0);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, 0);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0);
            bIsOpen = ImGui::CollapsingHeader(ComponentName);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF1C1C1C);

            ImGui::PopStyleColor(3);
                        
            ImGui::EndTable();
        }
        
        ImGui::PopStyleVar();
            
        if (!bIsRequired)
        {
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.2f, 0.2f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            
            //@TODO FIXME
            if (ImGui::SmallButton(LE_ICON_TRASH_CAN "##RemoveComponent"))
            {
                ComponentDestroyRequests.push(FComponentDestroyRequest{Table->GetType(), Entity});
            }
            
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Remove Component");
            }
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
        }
        
        if (bIsOpen)
        {
            ImGui::Spacing();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.015f, 0.015f, 0.015f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
            
            ImGui::Indent(4.0f);
            
            Table->DrawTree(World->IsSimulating());
            
            ImGui::Unindent(4.0f);
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(3);
            
            ImGui::Spacing();
        }
            
        ImGui::PopID();
    }

    void FWorldEditorTool::RemoveComponent(entt::entity Entity, const CStruct* ComponentType)
    {
        bool bWasRemoved = false;

        if (ComponentType == nullptr)
        {
            return;
        }
        
        ECS::Utils::ForEachComponent([&](void* Component, entt::basic_sparse_set<>& Set, const entt::meta_type& Type)
        {
            using namespace entt::literals;
            
            if (entt::meta_any ReturnValue = ECS::InvokeMetaFunc(Type, "static_struct"_hs))
            {
                CStruct* StructType = ReturnValue.cast<CStruct*>();
                
                if (StructType == ComponentType)
                {
                    Set.remove(Entity);
                    bWasRemoved = true;
                }
            }
        }, World->GetEntityRegistry(), Entity);
        
        
        if (bWasRemoved)
        {
            RebuildPropertyTables(Entity);
        }
        else
        {
            ImGuiX::Notifications::NotifyError("Failed to remove component: {0}", ComponentType->GetName().c_str());
        }
    }

    void FWorldEditorTool::DrawEmptyState()
    {
        ImVec2 WindowSize = ImGui::GetWindowSize();
        ImVec2 CenterPos = ImVec2(WindowSize.x * 0.5f, WindowSize.y * 0.5f);
    
        ImGui::SetCursorPos(ImVec2(CenterPos.x - 100.0f, CenterPos.y - 40.0f));
    
        ImGui::BeginGroup();
        {
            // Empty state icon
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        
            // Center the icon
            const char* EmptyIcon = LE_ICON_INBOX;
            ImVec2 IconSize = ImGui::CalcTextSize(EmptyIcon);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (200.0f - IconSize.x) * 0.5f);
        
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::TextUnformatted(EmptyIcon);
            ImGui::PopFont();
        
            ImGui::Spacing();
        
            const char* EmptyText = "Nothing selected";
            ImVec2 TextSize = ImGui::CalcTextSize(EmptyText);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (200.0f - TextSize.x) * 0.5f);
            ImGui::TextUnformatted(EmptyText);
        
            ImGui::Spacing();
        
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
            const char* HintText = "Select an entity to view properties";
            ImVec2 HintSize = ImGui::CalcTextSize(HintText);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (200.0f - HintSize.x) * 0.5f);
            ImGui::TextUnformatted(HintText);
            ImGui::PopStyleColor();
        
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();
    }

    void FWorldEditorTool::DrawEntityEditor(bool bFocused, entt::entity Entity)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        
        ImGui::BeginChild("Property Editor", ImVec2(0, 0), true);
    
        if (World->GetEntityRegistry().valid(Entity))
        {
            DrawEntityProperties(Entity);
        }
        else
        {
            DrawEmptyState();
        }
    
        ImGui::EndChild();
    
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void FWorldEditorTool::DrawPropertyEditor(bool bFocused)
    {
        
    }

    void FWorldEditorTool::RebuildPropertyTables(entt::entity Entity)
    {
        using namespace entt::literals;
        
        PropertyTables.clear();

        if (World->GetEntityRegistry().valid(Entity))
        {
            ECS::Utils::ForEachComponent([&](void* Component, entt::basic_sparse_set<>& Set, const entt::meta_type& Type)
            {
                entt::meta_any Any = ECS::InvokeMetaFunc(Type, "static_struct"_hs);
                if (!Any)
                {
                    return;
                }
                    
                CStruct* Struct = Any.cast<CStruct*>();
                    
                TUniquePtr<FPropertyTable> NewTable = MakeUniquePtr<FPropertyTable>(Component, Struct);
                PropertyTables.emplace_back(Move(NewTable))->RebuildTree();
                
                
            }, World->GetEntityRegistry(), Entity);
        }
    }

    void FWorldEditorTool::CreateEntityWithComponent(const CStruct* Component)
    {
        using namespace entt::literals;
        
        entt::entity CreatedEntity = entt::null;

        entt::hashed_string Hash = entt::hashed_string(Component->GetName().c_str());
        entt::meta_type MetaType = entt::resolve(Hash);
        
        CreatedEntity = World->ConstructEntity("Entity");
        World->GetEntityRegistry().get<SNameComponent>(CreatedEntity).Name = Component->GetName().ToString() + "_" + eastl::to_string((uint32)CreatedEntity);
        ECS::InvokeMetaFunc(MetaType, "emplace"_hs, entt::forward_as_meta(World->GetEntityRegistry()), CreatedEntity, entt::forward_as_meta(entt::meta_any{}));

        if (CreatedEntity != entt::null)
        {
            SetSelectedEntity(CreatedEntity);   
            OutlinerListView.MarkTreeDirty();
        }
    }

    void FWorldEditorTool::CreateEntity()
    {
        entt::entity NewEntity = World->ConstructEntity("Entity");
        SetSelectedEntity(NewEntity);   
        OutlinerListView.MarkTreeDirty();
    }

    void FWorldEditorTool::CopyEntity(entt::entity& To, entt::entity From)
    {
        World->CopyEntity(To, From);
        OutlinerListView.MarkTreeDirty();
    }

    void FWorldEditorTool::CycleGuizmoOp()
    {
        switch (GuizmoOp)
        {
        case ImGuizmo::TRANSLATE:
            {
                GuizmoOp = ImGuizmo::ROTATE;
            }
            break;
        case ImGuizmo::ROTATE:
            {
                GuizmoOp = ImGuizmo::SCALE;
            }
            break;
        case ImGuizmo::SCALE:
            {
                GuizmoOp = ImGuizmo::TRANSLATE;
            }
            break;
        }
    }
}
