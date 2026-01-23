#pragma once

namespace Lumina
{
    struct FWindowSpecs
    {
        FString Title = "Lumina";
        glm::uvec2 Extent = glm::uvec2(0);
        bool bFullscreen = false;
        bool bShowTitlebar = false;
    };
}
