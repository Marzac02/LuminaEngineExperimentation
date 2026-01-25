#include "PrefabEditorTool.h"


namespace Lumina
{
    FPrefabEditorTool::FPrefabEditorTool(IEditorToolContext* Context, CObject* InAsset)
        : FAssetEditorTool(Context, InAsset->GetName().ToString(), InAsset)
    {
        
    }

    void FPrefabEditorTool::OnInitialize()
    {
        FAssetEditorTool::OnInitialize();
    }

    void FPrefabEditorTool::SetupWorldForTool()
    {
        FAssetEditorTool::SetupWorldForTool();
    }

    void FPrefabEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        FAssetEditorTool::Update(UpdateContext);
    }

    void FPrefabEditorTool::OnDeinitialize(const FUpdateContext& UpdateContext)
    {
    }

    void FPrefabEditorTool::OnAssetLoadFinished()
    {
        FAssetEditorTool::OnAssetLoadFinished();
    }

    void FPrefabEditorTool::DrawToolMenu(const FUpdateContext& UpdateContext)
    {
        FAssetEditorTool::DrawToolMenu(UpdateContext);
    }

    void FPrefabEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGuiID leftDockID = 0, rightDockID = 0, bottomDockID = 0;

        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Right, 0.3f, &rightDockID, &leftDockID);

        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Down, 0.3f, &bottomDockID, &InDockspaceID);

        ImGui::DockBuilderDockWindow(GetToolWindowName(ViewportWindowName).c_str(), leftDockID);
        ImGui::DockBuilderDockWindow(GetToolWindowName("Foobar").c_str(), rightDockID);
    }
}
