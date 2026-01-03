#pragma once
#include "Core/Object/ObjectMacros.h"
#include "EnvironmentComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SSSAOInfo
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Category = "SSAO")
        float Radius = 1.0f;

        PROPERTY(Editable, Category = "SSAO")
        float Intensity = 2.0f;

        PROPERTY(Editable, Category = "SSAO")
        float Power = 1.5f;
    };

    REFLECT()
    struct LUMINA_API SAmbientLight
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Color, Category = "Ambient Light")
        glm::vec3 Color = glm::vec3(1.0f);

        PROPERTY(Editable, Category = "Ambient Light")
        float Intensity = 0.065f;
        
    };
    
    REFLECT(Component)
    struct LUMINA_API SEnvironmentComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Category = "Lighting")
        SAmbientLight AmbientLight;
        
        PROPERTY(Editable, Category = "SSAO")
        bool bSSAOEnabled = false;

        PROPERTY(Editable, Category = "SSAO")
        SSSAOInfo SSAOInfo;
        
    };
}
