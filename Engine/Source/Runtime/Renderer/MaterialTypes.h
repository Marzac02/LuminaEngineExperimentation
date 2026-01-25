#pragma once
#include "Core/Object/ObjectMacros.h"
#include "glm/glm.hpp"
#include "MaterialTypes.generated.h"

#define MAX_VECTORS 24
#define MAX_SCALARS 24


namespace Lumina
{
    struct FMaterialUniforms
    {
        glm::vec4 Scalars[MAX_SCALARS / 4];
        glm::vec4 Vectors[MAX_VECTORS];
    };
    

    REFLECT()
    enum class EMaterialParameterType : uint8
    {
        Scalar,
        Vector,
        Texture,
    };


    REFLECT()
    struct RUNTIME_API FMaterialParameter
    {
        GENERATED_BODY()
        
        PROPERTY()
        FName ParameterName;

        PROPERTY()
        EMaterialParameterType Type;

        PROPERTY()
        uint16 Index;
    };
    
}
