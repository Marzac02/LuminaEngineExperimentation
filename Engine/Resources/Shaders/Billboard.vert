#version 460 core

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"
#include "Includes/VertexInputs.glsl"

layout(location = 0) out vec2 vUV;

const vec2 Positions[6] = vec2[]
(
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

const vec2 UVs[6] = vec2[]
(
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main()
{
    vec2 QuadPos = Positions[gl_VertexIndex];
    vUV = UVs[gl_VertexIndex];

    vec3 BillboardCenter = vec3(1.0, 0.0, 15.0);
    float BillboardSize = 1.0;

    vec3 WorldUp = vec3(0.0, 1.0, 0.0);
    
    vec3 Forward    = normalize(GetCameraPosition() - BillboardCenter);
    vec3 Right      = normalize(cross(WorldUp, Forward));
    vec3 Up         = cross(Forward, Right);
    
    vec3 WorldPos   = BillboardCenter + (Right * QuadPos.x * BillboardSize) + (Up * QuadPos.y * BillboardSize);
    
    gl_Position     = GetCameraProjection() * GetCameraView() * vec4(WorldPos, 1.0);
}