#version 450 core
#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants
{
    uint Color;
    uint Thickness;
    uint NumSections;
    uint Selections[29];
};



void main() 
{
    ivec2 TexSize = textureSize(uSelectionTexture, 0);
    ivec2 PixelCoord = ivec2(inUV * vec2(TexSize));
    uint Selected = texelFetch(uSelectionTexture, PixelCoord, 0).g;
    
    if (Selected == 1)
    {
        bool Left  = bool(texelFetch(uSelectionTexture, PixelCoord + ivec2(-int(Thickness), 0), 0).g);
        bool Right = bool(texelFetch(uSelectionTexture, PixelCoord + ivec2( int(Thickness), 0), 0).g);
        bool Up    = bool(texelFetch(uSelectionTexture, PixelCoord + ivec2(0, -int(Thickness)), 0).g);
        bool Down  = bool(texelFetch(uSelectionTexture, PixelCoord + ivec2(0,  int(Thickness)), 0).g);

        if (!Left || !Right || !Up || !Down)
        {
            outColor = UnpackColor(Color);
            return;
        }
    }
    discard;
}