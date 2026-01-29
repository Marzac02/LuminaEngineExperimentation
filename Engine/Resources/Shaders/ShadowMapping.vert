#version 460

#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"
#include "Includes/VertexInputs.glsl"

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outLightPos;

layout(push_constant) uniform PushConstants
{
    uint LightIndex;
};

void main()
{
    vec3 Position = inPosition;

    FLight Light = LightData.Lights[LightIndex];
    mat4 ModelMatrix = GetModelMatrix(gl_InstanceIndex);

    #ifdef SKINNED_VERTEX
    vec4 Weights = vec4(inJointWeights) / 255.0;

    FInstanceData Instance = GetInstanceData(gl_InstanceIndex);
    uint BoneOffset = Instance.BoneOffset;
    
    mat4 SkinMatrix =
    BoneData.BoneMatrices[BoneOffset + inJointIndices.x] * Weights.x +
    BoneData.BoneMatrices[BoneOffset + inJointIndices.y] * Weights.y +
    BoneData.BoneMatrices[BoneOffset + inJointIndices.z] * Weights.z +
    BoneData.BoneMatrices[BoneOffset + inJointIndices.w] * Weights.w;

    Position = (SkinMatrix * vec4(inPosition, 1.0)).xyz;
    #endif

    vec4 vWorldPos = ModelMatrix * vec4(Position, 1.0);
    
    outWorldPos = vWorldPos.xyz;
    outLightPos = Light.Position;
    
    gl_Position = Light.ViewProjection[gl_ViewIndex] * vWorldPos;
}