#version 460 core

#extension GL_ARB_shader_draw_parameters : enable

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"
#include "Includes/VertexInputs.glsl"

// Outputs
layout(location = 0) out vec4 outFragColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outNormalWS;
layout(location = 3) out vec4 outFragPos;
layout(location = 4) out vec2 outUV;
layout(location = 5) flat out uint outEntityID;

precise invariant gl_Position;

//******* IMPORTANT *******
// Changes to any calculations to gl_Position here, must be exactly reflected in DepthPrePass.vert.

void main()
{
    vec3 Position = inPosition;
    vec3 NormalOS = UnpackNormal(inNormal);

    #ifdef SKINNED_VERTEX
    vec4 Weights = vec4(inJointWeights) / 255.0;

    FInstanceData Instance = GetInstanceData(gl_InstanceIndex);
    uint BoneOffset = Instance.BoneOffset;
    
    mat4 SkinMatrix =
    Bones.BoneMatrices[BoneOffset + inJointIndices.x] * Weights.x +
    Bones.BoneMatrices[BoneOffset + inJointIndices.y] * Weights.y +
    Bones.BoneMatrices[BoneOffset + inJointIndices.z] * Weights.z +
    Bones.BoneMatrices[BoneOffset + inJointIndices.w] * Weights.w;

    Position = (SkinMatrix * vec4(inPosition, 1.0)).xyz;
    NormalOS = mat3(SkinMatrix) * NormalOS;
    #endif
    
    vec2 uv = UnpackUV(inUV);

    mat4 ModelMatrix = GetModelMatrix(gl_InstanceIndex);
    mat4 View = GetCameraView();
    mat4 Projection = GetCameraProjection();

    vec4 WorldPos = ModelMatrix * vec4(Position, 1.0);
    vec4 ViewPos = View * WorldPos;
    

    // World-space
    mat3 NormalMatrixWS = transpose(inverse(mat3(ModelMatrix)));
    vec3 NormalWS = NormalMatrixWS * NormalOS;
    
    // View-space
    mat3 NormalMatrixVS = transpose(inverse(mat3(View * ModelMatrix)));
    vec3 NormalVS = NormalMatrixVS * NormalOS;

    FInstanceData InstanceData = GetInstanceData(gl_InstanceIndex);

    // Outputs
    outUV           = vec2(uv.x, uv.y);
    outFragPos      = ViewPos;
    outNormal       = vec4(NormalVS, 1.0);
    outNormalWS     = vec4(NormalWS, 1.0);
    outFragColor    = inColor;
    outEntityID     = InstanceData.EntityID;
    gl_Position     = Projection * ViewPos;
}
