#version 460

#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) in vec3 inPosition;      // RGB32_FLOAT
layout(location = 1) in uint inNormal;        // R32_UINT (packed 10:10:10:2)
layout(location = 2) in uvec2 inUV;           // RG16_UINT
layout(location = 3) in vec4 inColor;         // RGBA8_UNORM

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outLightPos;

layout(push_constant) uniform PushConstants
{
    uint LightIndex;
};

void main()
{
    FLight Light = LightData.Lights[LightIndex];
    mat4 ModelMatrix = GetModelMatrix(gl_InstanceIndex);
    vec4 vWorldPos = ModelMatrix * vec4(inPosition, 1.0);
    
    outWorldPos = vWorldPos.xyz;
    outLightPos = Light.Position;
    
    gl_Position = Light.ViewProjection[gl_ViewIndex] * vWorldPos;
}