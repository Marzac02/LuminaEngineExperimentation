#version 460 core

#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outPicker;

layout(set = 1, binding = 0) uniform sampler2D uBillboardTexture;

void main()
{
    vec4 Texture = texture(uBillboardTexture, vUV);
    if(Texture.a <= 0.0)
    {
        discard;
    }
    
    outColor = Texture;
}
