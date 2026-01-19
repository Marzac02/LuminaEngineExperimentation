#include "AnimationEditorTool.h"
#include "ImGuiDrawUtils.h"
#include "Assets/AssetTypes/Mesh/Animation/Animation.h"
#include "assets/assettypes/mesh/skeletalmesh/skeletalmesh.h"
#include "assets/assettypes/mesh/skeleton/skeleton.h"
#include "glm/gtx/string_cast.hpp"
#include "Tools/UI/ImGui/ImGuiFonts.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "world/entity/components/environmentcomponent.h"
#include "World/Entity/Components/LightComponent.h"
#include "world/Entity/Components/SimpleAnimationComponent.h"
#include "world/entity/components/skeletalmeshcomponent.h"
#include "World/Entity/Components/StaticMeshComponent.h"
#include "world/entity/components/velocitycomponent.h"


namespace Lumina
{
    static const char* MeshPropertiesName        = "MeshProperties";

    FAnimationEditorTool::FAnimationEditorTool(IEditorToolContext* Context, CObject* InAsset)
        : FAssetEditorTool(Context, InAsset->GetName().c_str(), InAsset, NewObject<CWorld>())
    {
    }

    void FAnimationEditorTool::OnInitialize()
    {
        CreateToolWindow(MeshPropertiesName, [&](bool bFocused)
        {
            ImGuiX::Font::PushFont(ImGuiX::Font::EFont::Large);
            ImGui::SeparatorText("Asset Details");
            ImGuiX::Font::PopFont();
            
            ImGui::Spacing();
            PropertyTable.DrawTree();
        });
    }

    void FAnimationEditorTool::SetupWorldForTool()
    {
        FEditorTool::SetupWorldForTool();
        
        DirectionalLightEntity = World->ConstructEntity("Directional Light");
        World->GetEntityRegistry().emplace<SDirectionalLightComponent>(DirectionalLightEntity);
        World->GetEntityRegistry().emplace<SEnvironmentComponent>(DirectionalLightEntity);
        
        CAnimation* Animation = Cast<CAnimation>(Asset.Get());
        if (!Animation->Skeleton.IsValid())
        {
            return;
        }
        
        World->GetEntityRegistry().get<SVelocityComponent>(EditorEntity).Speed = 5.0f;
        
        MeshEntity = World->ConstructEntity("MeshEntity");
        World->GetEntityRegistry().emplace<SSkeletalMeshComponent>(MeshEntity).SkeletalMesh = Animation->Skeleton->PreviewMesh;
        World->GetEntityRegistry().emplace<SSimpleAnimationComponent>(MeshEntity).Animation = Animation;
        STransformComponent& MeshTransform = World->GetEntityRegistry().get<STransformComponent>(MeshEntity);
        MeshTransform.SetLocation(glm::vec3(0.0f, 0.0f, 2.5));

        STransformComponent& EditorTransform = World->GetEntityRegistry().get<STransformComponent>(EditorEntity);

        glm::quat Rotation = Math::FindLookAtRotation(MeshTransform.GetLocation(), EditorTransform.GetLocation());
        EditorTransform.SetRotation(Rotation);
        
        World->MarkTransformDirty(EditorEntity);
    }

    void FAnimationEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        FAssetEditorTool::Update(UpdateContext);
        
        if (!World.IsValid())
        {
            return;
        }
        
    }

    void FAnimationEditorTool::OnDeinitialize(const FUpdateContext& UpdateContext)
    {
    }

    void FAnimationEditorTool::OnAssetLoadFinished()
    {
    }

    void FAnimationEditorTool::DrawToolMenu(const FUpdateContext& UpdateContext)
    {
        FAssetEditorTool::DrawToolMenu(UpdateContext);
        
        // Gizmo Control Dropdown
        if (ImGui::BeginMenu(LE_ICON_MOVE_RESIZE " Gizmo Control"))
        {
            const char* operations[] = { "Translate", "Rotate", "Scale" };
            static int currentOp = 0;

            if (ImGui::Combo("##", &currentOp, operations, IM_ARRAYSIZE(operations)))
            {
                switch (currentOp)
                {
                case 0: GuizmoOp = ImGuizmo::TRANSLATE; break;
                case 1: GuizmoOp = ImGuizmo::ROTATE;    break;
                case 2: GuizmoOp = ImGuizmo::SCALE;     break;
                }
            }

            ImGui::EndMenu();
        }
    }

    void FAnimationEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGuiID leftDockID = 0, rightDockID = 0, bottomDockID = 0;

        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Right, 0.3f, &rightDockID, &leftDockID);

        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Down, 0.3f, &bottomDockID, &InDockspaceID);

        ImGui::DockBuilderDockWindow(GetToolWindowName(ViewportWindowName).c_str(), leftDockID);
        ImGui::DockBuilderDockWindow(GetToolWindowName(MeshPropertiesName).c_str(), rightDockID);
    }
}
