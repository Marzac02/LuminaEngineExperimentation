#include "EditorToolModal.h"

#include "Core/Templates/LuminaTemplate.h"

namespace Lumina
{
    void FEditorModalManager::CreateDialogue(const FString& Title, ImVec2 Size, TMoveOnlyFunction<bool()> DrawFunction, bool bBlocking, bool bCloseable)
    {
        if (ActiveModal != nullptr)
        {
            return;
        }
        
        ActiveModal = MakeUnique<FEditorToolModal>(Title, Size, bCloseable);
        ActiveModal->DrawFunction = Move(DrawFunction);
        ActiveModal->bBlocking = bBlocking;
    }


    void FEditorModalManager::DrawDialogue()
    {
        if (!ActiveModal)
        {
            return;
        }

        if (ActiveModal->bBlocking)
        {
            ImGui::OpenPopup(ActiveModal->Title.c_str());
        }

        ImGuiViewport* Viewport = ImGui::GetMainViewport();
        ImVec2 Center = Viewport->GetCenter();
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ActiveModal->Size, ImGuiCond_Appearing);

        bool* bOpen = ActiveModal->bCloseable ? &ActiveModal->bOpen : nullptr;
        
        if (ActiveModal->bBlocking)
        {
            if (ImGui::BeginPopupModal(ActiveModal->Title.c_str(), bOpen, ImGuiWindowFlags_NoCollapse))
            {
                if (ActiveModal->DrawModal() || !ActiveModal->bOpen)
                {
                    ImGui::CloseCurrentPopup();
                    ActiveModal.reset();
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            if (ImGui::Begin(ActiveModal->Title.c_str(), bOpen, ImGuiWindowFlags_NoCollapse))
            {
                if (ActiveModal->DrawModal() || !ActiveModal->bOpen)
                {
                    ActiveModal.reset();
                }
                ImGui::End();
            }
        }
    }
}
