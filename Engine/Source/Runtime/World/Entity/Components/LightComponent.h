#pragma once

#include "glm/glm.hpp"
#include "LightComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SPointLightComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Color, Category = "Light")
        glm::vec3 LightColor = glm::vec3(1.0f);

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f)
        float Intensity = 10.0f;

        PROPERTY(Editable, Category = "Light")
        float Attenuation = 10.0f;

        PROPERTY(Editable, Category = "Light")
        float Falloff = 0.8f;
        
        PROPERTY(Editable, Category = "Shadows")
        bool bCastShadows = false;
    };

    REFLECT(Component)
    struct RUNTIME_API SSpotLightComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable, Color, Category = "Light")
        glm::vec3 LightColor = glm::vec3(1.0f);

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f, ClampMax = 1000.0f)
        float Intensity = 10.0f;

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f)
        float InnerConeAngle = 20.0f;

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f)
        float OuterConeAngle = 30.0f;

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f)
        float Attenuation = 10.0f;

        PROPERTY(Editable, Category = "Light")
        float Falloff = 0.8f;

        PROPERTY(Editable, Category = "Shadows")
        bool bCastShadows = false;

        PROPERTY(Editable, Category = "Shadows")
        float ShadowBias = 0.005f;

        PROPERTY(Editable, Category = "Shadows")
        float ShadowRadius = 1.0f;

        PROPERTY(Editable, Category = "Advanced")
        bool bVolumetric = false;

        PROPERTY(Editable, Category = "Advanced")
        float VolumetricIntensity = 0.5f;

        PROPERTY(NotSerialized)
        int32 ShadowMapIndex = -1;
    };


    REFLECT(Component)
    struct RUNTIME_API SDirectionalLightComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable, Color, Category = "Light")
        glm::vec3 Color = glm::vec4(1.0f);

        PROPERTY(Editable, Category = "Light")
        glm::vec3 Direction = glm::vec3(0.0f, 0.3f, 0.8f);

        PROPERTY(Editable, Category = "Light", ClampMin = 0.0f)
        float Intensity = 2.5f;

        PROPERTY(Editable)
        bool bCastShadows = true;
    };
}