
#include "EditorTool.h"
#include "imgui_internal.h"
#include "ToolFlags.h"
#include "EASTL/sort.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "World/WorldManager.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Components/EditorComponent.h"
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
            if (World->GetRenderer() == nullptr)
            {
                World->InitializeWorld();
            }
            
            SetupWorldForTool();
            
            Internal_CreateViewportTool();
        }

        OnInitialize();
    }

    void FEditorTool::Deinitialize(const FUpdateContext& UpdateContext)
    {
        OnDeinitialize(UpdateContext);
        
        for (FToolWindow* Window : ToolWindows)
        {
            Memory::Delete(Window);
        }
        
        if (HasWorld())
        {
            World->ShutdownWorld();
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
            World->ShutdownWorld();
            World->ForceDestroyNow();
            World = nullptr;
        }
        
        World = InWorld;

        if (World->GetRenderer() == nullptr)
        {
            World->InitializeWorld();
        }
        
        SetupWorldForTool();
    }

    void FEditorTool::SetupWorldForTool()
    {
        EditorEntity = World->ConstructEntity("Editor Entity");
        World->GetEntityRegistry().emplace<SCameraComponent>(EditorEntity);
        World->GetEntityRegistry().emplace<FHideInSceneOutliner>(EditorEntity);
        World->GetEntityRegistry().emplace<FEditorComponent>(EditorEntity);
        World->GetEntityRegistry().emplace<SVelocityComponent>(EditorEntity).Speed = 50.0f;
        World->GetEntityRegistry().get<STransformComponent>(EditorEntity).SetLocation(glm::vec3(0.0f, 0.0f, 1.5f));
        
        World->SetActiveCamera(EditorEntity);
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
        ImGuiX::ItemTooltip( "Undo" );

        //-------------------------------------------------------------------------

        
        if (ImGui::MenuItem(LE_ICON_REDO_VARIANT"##Redo"))
        {
            
        }
        ImGuiX::ItemTooltip( "Redo" );
        ImGui::EndDisabled();

        if (ImGui::BeginMenu(LE_ICON_HELP_CIRCLE_OUTLINE" Help"))
        {
            if (ImGui::BeginTable( "HelpTable", 2 ))
            {
                DrawHelpMenu(UpdateContext);
                ImGui::EndTable();
            }
            ImGui::EndMenu();
        }

        DrawToolMenu(UpdateContext);
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
        ImGui::Dummy(ImVec2(0, 0));
        DrawViewportOverlayElements(UpdateContext, ViewportTexture, ViewportSize);

        ImGui::Dummy(ImStyle.ItemSpacing);
        ImGui::SetCursorPos(ImStyle.ItemSpacing);
        ImGui::Dummy(ImVec2(0, 0));
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
        FToolWindow* Tool = Internal_CreateToolWindow(ViewportWindowName, nullptr);
        Tool->bViewport = true;
    }

    FEditorTool::FToolWindow* FEditorTool::Internal_CreateToolWindow(const FString& InName, const TFunction<void(const FUpdateContext&, bool)>& DrawFunction, const ImVec2& WindowPadding, bool DisableScrolling)
    {
        for (FToolWindow* Window : ToolWindows)
        {
            Assert(Window->Name != InName)
        }

        auto pToolWindow = ToolWindows.emplace_back(Memory::New<FToolWindow>(InName, DrawFunction, WindowPadding, DisableScrolling));
        eastl::sort(ToolWindows.begin(), ToolWindows.end(), [] (const FToolWindow* pLHS, const FToolWindow* pRHS)
        {
            return pLHS->Name < pRHS->Name;
        });
        
        return pToolWindow;
    }
    
    void FEditorTool::DrawHelpTextRow(const char* pLabel, const char* pText) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGui::Text(pLabel);
        }

        ImGui::TableNextColumn();
        {
            ImGui::Text(pText);
        }
    }
}
