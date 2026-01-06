#pragma once



#ifndef REFLECTION_PARSER

#include "Core/Math/Math.h"
#include "Core/Math/Transform.h"
#include "glm/glm.hpp"

#endif


#ifdef REFLECTION_PARSER

#include "ObjectMacros.h"

namespace glm
{
    
    REFLECT()
    struct vec2
    {
        PROPERTY(Editable)
        float x;

        PROPERTY(Editable)
        float y;
    };

    REFLECT()
    struct vec3
    {
        PROPERTY(Editable)
        float x;

        PROPERTY(Editable)
        float y;
    
        PROPERTY(Editable)
        float z;
    };

    REFLECT()
    struct vec4
    {
        PROPERTY(Editable)
        float x;

        PROPERTY(Editable)
        float y;
    
        PROPERTY(Editable)
        float z;

        PROPERTY(Editable)
        float w;
    };

    REFLECT()
    struct quat
    {
        PROPERTY(Editable)
        float x;

        PROPERTY(Editable)
        float y;
    
        PROPERTY(Editable)
        float z;

        PROPERTY(Editable)
        float w;
    };
}

namespace Lumina
{
    REFLECT()
    struct FTransform
    {
        PROPERTY(Editable)
        glm::vec3 Location;

        PROPERTY(Editable)
        glm::quat Rotation;

        PROPERTY(Editable)
        glm::vec3 Scale;
    };
}

#endif