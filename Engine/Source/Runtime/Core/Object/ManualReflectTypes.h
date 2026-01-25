#pragma once

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

#endif