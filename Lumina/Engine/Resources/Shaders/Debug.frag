#version 450 core
#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(set = 1, binding = 0) uniform sampler2D uDepth;
layout(set = 1, binding = 1) uniform sampler2DArray uShadowAtlas;
layout(set = 1, binding = 2) uniform sampler2DArray uShadowCascades;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 OutFragColor;


#define DEBUG_NONE              0
#define DEBUG_POSITION          1
#define DEBUG_NORMALS           2
#define DEBUG_ALBEDO            3
#define DEBUG_SSAO              4
#define DEBUG_AMBIENT_OCCLUSION 5
#define DEBUG_ROUGHNESS         6
#define DEBUG_METALLIC          7
#define DEBUG_SPECULAR          8
#define DEBUG_DEPTH             9
#define DEBUG_SHADOW_ATLAS      10
#define DEBUG_CASCADES          11

layout(push_constant) uniform DebugInfo
{
    uint DebugFlags;
} Debug;

void main()
{
    if (Debug.DebugFlags == DEBUG_POSITION)
    {
        float Depth = texture(uDepth, vUV).r;
        vec3 PositionVS = ReconstructViewPos(vUV, Depth, GetInverseCameraProjection());
        OutFragColor = vec4(PositionVS, 1.0);
        return;
    }
    
    if (Debug.DebugFlags == DEBUG_DEPTH)
    {
        float depth = texture(uDepth, vUV).r;
        float LinearDepth = LinearizeDepth(depth, GetFarPlane(), GetNearPlane());
        float VisualizedDepth = LinearDepth / GetFarPlane();

        OutFragColor = vec4(vec3(VisualizedDepth), 1.0);

        return;
    }

    if (Debug.DebugFlags == DEBUG_SHADOW_ATLAS)
    {
        int layerCount = 7;

        float fCount = float(layerCount);
        int cols = int(ceil(sqrt(fCount)));
        int rows = int(ceil(fCount / float(cols)));

        vec2 gridUV = vUV * vec2(cols, rows);
        ivec2 cell = ivec2(gridUV);
        vec2 cellUV = fract(gridUV);

        int layer = cell.y * cols + cell.x;

        if (layer < layerCount)
        {
            float s = texture(uShadowAtlas, vec3(cellUV, float(layer))).r;
            OutFragColor = vec4(vec3(s), 1.0);
        }
        else
        {
            OutFragColor = vec4(0.0);
        }
        return;
    }

    if(Debug.DebugFlags == DEBUG_CASCADES)
    {
        int LayerCount = 4;
        int Cols = int(ceil(sqrt((float(LayerCount)))));
        int Rows = int(ceil((float(LayerCount) / float(Cols))));

        vec2 GridUV = vUV * vec2(Cols, Rows);
        ivec2 Cell = ivec2(GridUV);
        vec2 CellUV = fract(GridUV);

        int Layer = Cell.y * Cols + Cell.x;

        if(Layer < LayerCount)
        {
            float Depth = texture(uShadowCascades, vec3(CellUV, float(Layer))).r;
            float LinearDepth = LinearizeDepth(Depth, GetNearPlane(), 1000.0f);
            float VisualizedDepth = LinearDepth / GetNearPlane();
            OutFragColor = vec4(vec3(Depth), 1.0);
        }
        else
        {
            OutFragColor = vec4(0.0);
        }
    }

    OutFragColor.a = 1.0;
}
