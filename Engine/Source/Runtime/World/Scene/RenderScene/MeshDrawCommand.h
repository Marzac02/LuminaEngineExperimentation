#pragma once
#include "Platform/GenericPlatform.h"


namespace Lumina
{
    class FRHIInputLayout;
    class FRHIBuffer;
    class CMaterialInterface;
    
    
    struct FDrawKey
    {
        uint64 StartIndex;
        uint64 IndexCount;
        
        bool operator == (const FDrawKey& Key) const
        {
            return StartIndex == Key.StartIndex && IndexCount == Key.IndexCount;
        }
    };

    static uint64 GetTypeHash(const FDrawKey& K)
    {
        size_t Seed = 0;
        Hash::HashCombine(Seed, K.StartIndex);
        Hash::HashCombine(Seed, K.IndexCount);
        return Seed;
    }


    /**
     * A mesh draw command fully encapsulates all the data needed to draw a mesh draw call.
     * Mesh draw commands are cached in the scene.
     */
    struct FMeshDrawCommand
    {
        FRHIVertexShader*                          VertexShader = nullptr;
        FRHIPixelShader*                           PixelShader = nullptr;
        uint32                                     IndirectDrawOffset = 0;
        THashMap<FDrawKey, uint32>                 DrawArgumentIndexMap;
        uint32                                     DrawCount = 0;
    };
}
