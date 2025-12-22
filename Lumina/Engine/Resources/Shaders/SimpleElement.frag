#version 450 core

#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) in vec3 inColor;
layout(location = 0) out vec3 outFragColor;

void main()
{
    outFragColor = inColor.rgb;
}