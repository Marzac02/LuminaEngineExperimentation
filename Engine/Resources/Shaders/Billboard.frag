#version 460 core

#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) out vec4 vColor;

void main()
{
    vColor = vec4(1.0, 0.0, 0.0, 1.0);
}
