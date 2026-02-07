
#include "EditorTool.h"
#include "imgui_internal.h"
#include "ToolFlags.h"
#include "Thumbnails/ThumbnailManager.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "World/WorldManager.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Components/EditorComponent.h"
#include "World/Entity/Components/InputComponent.h"
#include "World/Entity/Components/StaticMeshComponent.h"
#include "World/Entity/Components/VelocityComponent.h"
#include "World/Entity/Systems/EditorEntityMovementSystem.h"

namespace Lumina
{
    FEditorTool::FEditorTool(IEditorToolContext* Context, const FString& DisplayName, CWorld* InWorld)
        : ToolContext(Context)
        , ToolName(DisplayName)
        , World(InWorld)
        , EditorEntity(entt::null)
    {
        ToolFlags |= EEditorToolFlags::Tool_WantsToolbar;
    }

    void FEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGui::DockBuilderRemoveNodeChildNodes(InDockspaceID);
    }

    void FEditorTool::Initialize()
    {
        ToolName = std::format("{0} {1}", GetTitlebarIcon(), GetToolName().c_str()).c_str();

        if (HasWorld())
        {
            if (World->GetPhysicsScene() == nullptr)
            {
                World->InitializeWorld(EWorldType::Tool);
            }
            
            SetupWorldForTool();
            
            Internal_CreateViewportTool();
        }

        OnInitialize();
    }

    void FEditorTool::Deinitialize(const FUpdateContext& UpdateContext)
    {
        OnDeinitialize(UpdateContext);
        
        ToolWindows.clear();
        
        if (HasWorld())
        {
            World->TeardownWorld();
            World->ForceDestroyNow();
            World = nullptr;
        }
        
        ToolWindows.clear();
    }

    void FEditorTool::SetWorld(CWorld* InWorld)
    {
        if (World == InWorld)
        {
            return;
        }
        
        if (World.IsValid())
        {
            World->TeardownWorld();
            World->ForceDestroyNow();
            World = nullptr;
        }
        
        World = InWorld;

        if (World->GetPhysicsScene() == nullptr)
        {
            World->InitializeWorld(EWorldType::Tool);
        }
        
        SetupWorldForTool();
    }

    void FEditorTool::SetupWorldForTool()
    {
        EditorEntity = World->ConstructEntity("Editor Entity");
        World->GetEntityRegistry().emplace<FHideInSceneOutliner>(EditorEntity);
        World->GetEntityRegistry().emplace<SCameraComponent>(EditorEntity);
        World->GetEntityRegistry().emplace<SInputComponent>(EditorEntity);
        World->GetEntityRegistry().emplace<FEditorComponent>(EditorEntity);
        World->GetEntityRegistry().emplace<SVelocityComponent>(EditorEntity).Speed = 50.0f;
        World->GetEntityRegistry().get<STransformComponent>(EditorEntity).SetLocation(glm::vec3(0.0f, 1.25f, 3.25f));

        World->SetActiveCamera(EditorEntity);
    }

    entt::entity FEditorTool::CreateFloorPlane(float ScaleX, float ScaleY)
    {
        FTransform Transform;
        Transform.Rotate({-90.0f, 0.0f, 0.0f});
        Transform.Scale = glm::vec3(ScaleX, ScaleY, 1.0f);
        entt::entity FloorEntity = World->ConstructEntity("FloorPlane", Transform);
        World->GetEntityRegistry().emplace<FHideInSceneOutliner>(FloorEntity);
        SStaticMeshComponent& MeshComponent = World->GetEntityRegistry().emplace<SStaticMeshComponent>(FloorEntity);
        MeshComponent.StaticMesh = CThumbnailManager::Get().PlaneMesh;
        return FloorEntity;
    }

    void FEditorTool::DrawMainToolbar(const FUpdateContext& UpdateContext)
    {
        if (ImGui::MenuItem(LE_ICON_FILE_PLUS_OUTLINE"##New"))
        {
            OnNew();
        }

        if (IsAssetEditorTool())
        {
            if (ImGui::MenuItem(LE_ICON_CONTENT_SAVE"##Save"))
            {
                OnSave();
            }
        }

        ImGui::BeginDisabled();
        if (ImGui::MenuItem(LE_ICON_UNDO_VARIANT"##Undo"))
        {
            OnUndo();
        }
        ImGuiX::ItemTooltip("Undo");

        //-------------------------------------------------------------------------
        
        if (ImGui::MenuItem(LE_ICON_REDO_VARIANT"##Redo"))
        {
            
        }
        ImGuiX::ItemTooltip("Redo");
        ImGui::EndDisabled();

        if (ImGui::BeginMenu(LE_ICON_HELP_CIRCLE_OUTLINE" Help"))
        {
            if (ImGui::BeginTable("HelpTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                DrawHelpMenu();
                ImGui::EndTable();
            }
            ImGui::EndMenu();
        }

        DrawToolMenu(UpdateContext);
    }

    void FEditorTool::DrawViewportOverlayElements(const FUpdateContext& UpdateContext, ImTextureRef ViewportTexture, ImVec2 ViewportSize)
    {
        ImGui::Dummy(ImVec2(0, 0));
    }

    bool FEditorTool::DrawViewport(const FUpdateContext& UpdateContext, ImTextureRef ViewportTexture)
    {
        const ImVec2 ViewportSize(eastl::max(ImGui::GetContentRegionAvail().x, 64.0f), eastl::max(ImGui::GetContentRegionAvail().y, 64.0f));
        const ImVec2 WindowPosition = ImGui::GetWindowPos();
        const ImVec2 WindowBottomRight = { WindowPosition.x + ViewportSize.x, WindowPosition.y + ViewportSize.y };
        float AspectRatio = (ViewportSize.x / ViewportSize.y);
        float t = (ViewportSize.x - 500) / (1200 - 500);
        t = glm::clamp(t, 0.0f, 1.0f);
        float NewFOV = glm::mix(120.0f, 50.0f, t);

        if (SCameraComponent* CameraComponent =  World->GetActiveCamera())
        {
            CameraComponent->SetAspectRatio(AspectRatio);
            CameraComponent->SetFOV(NewFOV);
        }
        
        /** Mostly for debug, so we can easily see if there's some transparency issue */
        ImGui::GetWindowDrawList()->AddRectFilled(WindowPosition, WindowBottomRight, IM_COL32(255, 0, 0, 255));
        
        
        if (bViewportHovered)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                ImGui::SetWindowFocus();
                bViewportFocused = true;
            }
        }

        ImVec2 CursorScreenPos = ImGui::GetCursorScreenPos();
        
        ImGui::GetWindowDrawList()->AddImage(
            ViewportTexture,
            CursorScreenPos,
            ImVec2(CursorScreenPos.x + ViewportSize.x, CursorScreenPos.y + ViewportSize.y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32_WHITE
        );

        const ImGuiStyle& ImStyle = ImGui::GetStyle();

        ImGui::Dummy(ImStyle.ItemSpacing);
        ImGui::SetCursorPos(ImStyle.ItemSpacing);
        
        DrawViewportOverlayElements(UpdateContext, ViewportTexture, ViewportSize);

        ImGui::Dummy(ImStyle.ItemSpacing);
        ImGui::SetCursorPos(ImStyle.ItemSpacing);
        
        DrawViewportToolbar(UpdateContext);
        
        if (ImGuiDockNode* pDockNode = ImGui::GetWindowDockNode())
        {
           pDockNode->LocalFlags = 0;
           pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
           pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }

        return false;
    }

    void FEditorTool::DrawViewportToolbar(const FUpdateContext& UpdateContext)
    {
        ImGui::Dummy(ImVec2(0, 0));
    }

    void FEditorTool::FocusViewportToEntity(entt::entity Entity)
    {
        if (!HasWorld())
        {
            return;
        }
    
        if (!World->GetEntityRegistry().valid(Entity))
        {
            return;
        }
    
        const STransformComponent& EntityTransform = World->GetEntityRegistry().get<STransformComponent>(Entity);
        STransformComponent& EditorTransform = World->GetEntityRegistry().get<STransformComponent>(EditorEntity);
        
        float FocusDistance = 3.0f;
    
        glm::vec3 CurrentForward = EditorTransform.GetForward();
        glm::vec3 NewPosition = EntityTransform.GetLocation() - CurrentForward * FocusDistance;
        EditorTransform.SetLocation(NewPosition);
        
        glm::quat Rotation = Math::FindLookAtRotation(EntityTransform.GetLocation(), NewPosition);
        EditorTransform.SetRotation(Rotation);
    
        World->MarkTransformDirty(EditorEntity);
    }

    bool FEditorTool::BeginViewportToolbarGroup(char const* GroupID, ImVec2 GroupSize, const ImVec2& Padding)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xFF2C2C2C);
        ImGui::PushStyleColor(ImGuiCol_Header, 0xFF2C2C2C);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF2C2C2C);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0xFF303030);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0xFF3A3A3A);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Padding);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

        // Adjust "use available" height to default toolbar height
        if (GroupSize.y <= 0)
        {
            GroupSize.y = ImGui::GetFrameHeight();
        }

        return ImGui::BeginChild(GroupID, GroupSize, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    }

    void FEditorTool::EndViewportToolbarGroup()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
    }

    void FEditorTool::Internal_CreateViewportTool()
    {
        FToolWindow* Tool = CreateToolWindow(ViewportWindowName, nullptr);
        Tool->bViewport = true;
    }

    FEditorTool::FToolWindow* FEditorTool::CreateToolWindow(FName InName, const TFunction<void(bool)>& DrawFunction, const ImVec2& WindowPadding, bool DisableScrolling)
    {
        DEBUG_ASSERT(eastl::none_of(ToolWindows.begin(), ToolWindows.end(), [&](const TUniquePtr<FToolWindow>& W)
        {
            return W->Name == InName;
        }));
        
        auto ToolWindow = MakeUnique<FToolWindow>(InName, DrawFunction, WindowPadding, DisableScrolling); 
        return ToolWindows.emplace_back(Move(ToolWindow)).get();
    }
    
    void FEditorTool::DrawHelpTextRow(const char* Label, const char* Text) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGui::TextUnformatted(Label);
        }

        ImGui::TableNextColumn();
        {
            ImGui::TextUnformatted(Text);
        }
    }
}
