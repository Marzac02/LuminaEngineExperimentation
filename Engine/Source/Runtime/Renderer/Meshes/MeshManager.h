#pragma once
#include "Renderer/RenderResource.h"

namespace Lumina
{
    struct FMeshResource;

    class FMeshManager
    {
                
        FMeshManager() = default;
        ~FMeshManager() = default;
        
    public:

        FMeshManager(const FMeshManager&) = delete;
        FMeshManager& operator=(const FMeshManager&) = delete;
        
        static FMeshManager& Get();
    
        void EmplaceMeshResource(FMeshResource& Resource);
        
    private:
        
        void Initialize();
        
    
    private:
        
        FRHIBufferRef VertexBuffer;
        FRHIBufferRef IndexBuffer;
    };
}
