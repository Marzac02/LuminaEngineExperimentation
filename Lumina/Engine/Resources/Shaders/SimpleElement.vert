#version 450 core

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

// Input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inColor;

layout(location = 0) out vec4 outColor;


void main()
{
    outColor = UnpackColor(inColor);
    
    gl_Position = GetCameraProjection() * GetCameraView() * vec4(inPosition, 1.0);
}