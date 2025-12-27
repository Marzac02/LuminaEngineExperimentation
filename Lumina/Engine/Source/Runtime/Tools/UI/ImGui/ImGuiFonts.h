#pragma once

#include "imgui.h"
#include "Containers/String.h"
#include "Module/API.h"


namespace Lumina::ImGuiX::Font
{
    enum class EFont : uint8
    {
        Tiny,
        TinyBold,
        Small,
        SmallBold,
        Medium,
        MediumBold,
        Large,
        LargeBold,

        NumFonts,
        Default = Medium,
    };
    
    LUMINA_API extern ImFont* GFonts[static_cast<int32>(EFont::NumFonts)];

    LUMINA_API FORCEINLINE void PushFont(EFont font) 
    {
        ImFont* Font = GFonts[static_cast<int8>(font)];
        ImGui::PushFont(Font); 
    }

    LUMINA_API FORCEINLINE void PopFont() { ImGui::PopFont(); }
}

