#version 450 core
#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform usampler2D uSelectionTexture;

layout(push_constant) uniform PushConstants
{
    uint Color;
    uint Thickness;
    uint Selection;
};

void main() 
{
    ivec2 TexSize = textureSize(uSelectionTexture, 0);
    ivec2 PixelCoord = ivec2(inUV * vec2(TexSize));
    
    uint CenterID = texelFetch(uSelectionTexture, PixelCoord, 0).r;
    
    if(CenterID == Selection) 
    {
        uint Left  = texelFetch(uSelectionTexture, PixelCoord + ivec2(-Thickness, 0), 0).r;
        uint Right = texelFetch(uSelectionTexture, PixelCoord + ivec2(Thickness, 0), 0).r;
        uint Up    = texelFetch(uSelectionTexture, PixelCoord + ivec2(0, -Thickness), 0).r;
        uint Down  = texelFetch(uSelectionTexture, PixelCoord + ivec2(0, Thickness), 0).r;
        
        if(Left != CenterID || Right != CenterID || Up != CenterID || Down != CenterID)
        {
            vec4 FinalColor = UnpackColor(Color);
            outColor = FinalColor;
            return;
        }
    }

    discard;
}