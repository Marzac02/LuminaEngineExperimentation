#include "EditorToolModal.h"

#include "Core/Templates/LuminaTemplate.h"

namespace Lumina
{
    FEditorModalManager::~FEditorModalManager()
    {

    }

    void FEditorModalManager::CreateDialogue(const FString& Title, ImVec2 Size, TMoveOnlyFunction<bool()> DrawFunction, bool bBlocking)
    {
        if (ActiveModal != nullptr)
        {
            return;
        }
        
        ActiveModal = MakeUnique<FEditorToolModal>(Title, Size);
        ActiveModal->DrawFunction = Move(DrawFunction);
        ActiveModal->bBlocking = bBlocking;
        
    }


    void FEditorModalManager::DrawDialogue()
    {
        if (!ActiveModal)
        {
            return;
        }

        // Only open popups for blocking modals
        if (ActiveModal->bBlocking)
        {
            ImGui::OpenPopup(ActiveModal->Title.c_str());
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 center = viewport->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ActiveModal->Size, ImGuiCond_Appearing);

        if (ActiveModal->bBlocking)
        {
            bool bOpen = true;
            if (ImGui::BeginPopupModal(ActiveModal->Title.c_str(), &bOpen, ImGuiWindowFlags_NoResize))
            {
                if (ActiveModal->DrawModal() || !bOpen)
                {
                    ImGui::CloseCurrentPopup();
                    ActiveModal.reset();
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            bool bOpen = true;
            if (ImGui::Begin(ActiveModal->Title.c_str(), &bOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                if (ActiveModal->DrawModal() || !bOpen)
                {
                    ActiveModal.reset();
                }
                ImGui::End();
            }
        }
    }
}
