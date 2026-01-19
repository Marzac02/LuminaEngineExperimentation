#pragma once
#include "Platform/GenericPlatform.h"


namespace Lumina
{
    class FRHIInputLayout;
    class FRHIBuffer;
    class CMaterialInterface;


    /**
     * A mesh draw command fully encapsulates all the data needed to draw a mesh draw call.
     * Mesh draw commands are cached in the scene.
     */
    struct FMeshDrawCommand
    {
        FRHIVertexShader*   VertexShader = nullptr;
        FRHIPixelShader*    PixelShader = nullptr;
        FRHIBuffer*         IndexBuffer = nullptr;
        FRHIBuffer*         VertexBuffer = nullptr;
        FRHIBindingLayout*  BindingLayout = nullptr;
        FRHIBindingSet*     BindingSet = nullptr;
        FRHIInputLayout*    InputLayout = nullptr;
        uint32              IndirectDrawOffset = 0;
        bool                bSkinned = false;
    };
}
