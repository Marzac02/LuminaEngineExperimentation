#include "CoreTypeCustomization.h"
#include "imgui.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Class.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "Core/Reflection/Type/Properties/ObjectProperty.h"
#include "Paths/Paths.h"
#include "Renderer/RenderManager.h"
#include "thumbnails/thumbnailmanager.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "UI/EditorUI.h"

namespace Lumina
{
    static constexpr ImVec2 GButtonSize(42, 0);

    EPropertyChangeOp FCObjectPropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        FObjectProperty* ObjectProperty = static_cast<FObjectProperty*>(Property->Property);
        
        bool bWasChanged = false;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

        ImGui::PushID(this);
        if (ImGui::BeginChild("OP", ImVec2(-1, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            const auto& Style = ImGui::GetStyle();

            TObjectPtr<CObject> HardObject = Object.Lock();

            const char* Label = HardObject.IsValid() ? Object.Get()->GetName().c_str() : "<None>";
            ImGui::BeginDisabled(Object == nullptr);

            TOptional<ImTextureRef> ButtonTexture;
            if (Object.IsValid())
            {
                //FString FullPath = Paths::ResolveVirtualPath(Object.Get()->GetPathName());
                //CThumbnailManager::Get().TryLoadThumbnailsForPackage(FullPath);
                //if (FRHIImage* Image = CThumbnailManager::GetThumbnailForPackage(Object.Get()->GetPackage())->LoadedImage)
                //{
                //    ButtonTexture = ImGuiX::ToImTextureRef(Image);
                //}
            }


            if (!ButtonTexture.has_value())
            {
                ButtonTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/SkeletalMeshIcon.png");
            }

            
            ImGui::ImageButton(Label, ButtonTexture.value(), ImVec2(64, 64));
            ImGui::EndDisabled();

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                
            }

            ImGui::SameLine();

            ImGui::BeginGroup();

            float const ComboArrowWidth = ImGui::GetFrameHeight();
            float const TotalPathWidgetWidth = ImGui::GetContentRegionAvail().x;
            float const TextWidgetWidth = TotalPathWidgetWidth - ComboArrowWidth;

            ImGui::SetNextItemWidth(TextWidgetWidth);
            
            const bool bHasObject = Object != nullptr;

            FString PathString = bHasObject ? Object.Get()->GetName().ToString() : FString("<None>");
            char PathBuffer[512];
            strncpy(PathBuffer, PathString.c_str(), sizeof(PathBuffer) - 1);
            PathBuffer[sizeof(PathBuffer) - 1] = '\0';
        
            ImGui::PushStyleColor(ImGuiCol_Text, bHasObject ? ImVec4(0.6f, 0.6f, 0.6f, 1.0f) : ImVec4(1.0f, 0.19f, 0.19f, 1.0f));
        
            ImGui::InputText("##ObjectPathText", PathBuffer, sizeof(PathBuffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        
            ImGuiX::ItemTooltip(PathString.c_str());
        
            ImGui::PopStyleColor();

            const ImVec2 ComboDropDownSize(Math::Max(TotalPathWidgetWidth, 500.0f), ImGui::GetFrameHeight() * 20);
            ImGui::SetNextWindowSizeConstraints(ImVec2(ComboDropDownSize.x, 0), ComboDropDownSize);

            ImGui::SameLine(0, 0);
        
            bool bComboOpen = ImGui::BeginCombo("##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview);

            if (bComboOpen)
            {
                const float CursorPosYPostFilter = ImGui::GetCursorPosY();
                const float FilterHeight = CursorPosYPostFilter;

                const ImVec2 PreviousCursorPos = ImGui::GetCursorPos();
                const ImVec2 ChildSize(ImGui::GetContentRegionAvail().x, ComboDropDownSize.y - FilterHeight - Style.ItemSpacing.y - Style.WindowPadding.y);
                ImGui::Dummy(ChildSize);
                ImGui::SetCursorPos(PreviousCursorPos);

                SearchFilter.Draw("##Search", ImGui::GetContentRegionAvail().x);
                
                if (ImGui::BeginChild("##OptList", ChildSize, false, ImGuiChildFlags_NavFlattened))
                {
                    TVector<FAssetData> Assets = FAssetQuery().OfClass(ObjectProperty->GetPropertyClass()->GetName()).Execute();
                    
                    for (const FAssetData& Asset : Assets)
                    {
                        if (!SearchFilter.PassFilter(Asset.AssetName.c_str()))
                        {
                            continue;
                        }
                        
                        if (ImGui::Selectable(Asset.AssetName.c_str()))
                        {
                            Object = LoadObject<CObject>(Asset.AssetGUID);
                            ImGui::CloseCurrentPopup();
                        
                            bWasChanged = true;
                        }
                    }
                }
                
                ImGui::EndChild();
                ImGui::EndCombo();
            }
        
            ImGui::BeginDisabled(Object == nullptr);
            if (ImGui::Button(LE_ICON_CONTENT_COPY "##Copy", GButtonSize))
            {
                ImGui::SetClipboardText(HardObject->GetName().c_str());
            }

            ImGui::SameLine();

            if (ImGui::Button(LE_ICON_COG "##Options", GButtonSize))
            {
                
            }

            ImGui::SameLine();
        

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));

            if (ImGui::Button(LE_ICON_CLOSE_CIRCLE "##Clear", GButtonSize))
            {
                Object.Reset();
                bWasChanged = true;
            }
        
            ImGui::PopStyleColor(3);


            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
        ImGui::EndChild();

        ImGui::PopID();

        ImGui::PopItemWidth();
        
        return bWasChanged ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FCObjectPropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        TObjectPtr<CObject> Value = Object.Lock();
        Property->Property->SetValue(Property->ContainerPtr, Value, Property->Index);
    }

    void FCObjectPropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        TObjectPtr<CObject> Value;
        Property->Property->GetValue(Property->ContainerPtr, &Value, Property->Index);
        Object = Value;
    }
}
