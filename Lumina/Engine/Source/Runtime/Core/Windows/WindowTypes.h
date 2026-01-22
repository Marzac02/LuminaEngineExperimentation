#pragma once

#include "Events/Event.h"

namespace Lumina
{
    struct FWindowSpecs
    {
        FString Title = "Lumina";
        glm::uvec2 Extent = glm::uvec2(0);
        bool bFullscreen = false;
    };
}
