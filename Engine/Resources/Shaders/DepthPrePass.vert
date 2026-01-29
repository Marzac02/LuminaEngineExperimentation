#version 460 core
#extension GL_ARB_shader_draw_parameters : enable

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"
#include "Includes/VertexInputs.glsl"

precise invariant gl_Position;

//******* IMPORTANT *******
// Changes to any calculations to gl_Position here, must be exactly reflected in GeometryPass.vert.

void main()
{
    vec3 Position = inPosition;
    
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
    
    mat4 ModelMatrix = GetModelMatrix(gl_InstanceIndex);
    mat4 View = GetCameraView();
    mat4 Projection = GetCameraProjection();

    vec4 WorldPos = ModelMatrix * vec4(Position, 1.0);
    vec4 ViewPos = View * WorldPos;
    
    gl_Position = Projection * ViewPos;
}
