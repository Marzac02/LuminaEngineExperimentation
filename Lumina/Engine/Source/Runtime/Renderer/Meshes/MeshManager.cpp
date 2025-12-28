#include "pch.h"
#include "MeshManager.h"

#include "Renderer/MeshData.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"
#include "Renderer/Vertex.h"


namespace Lumina
{
    FMeshManager& FMeshManager::Get()
    {
        static FMeshManager Instance;
        return Instance;
    }

    void FMeshManager::EmplaceMeshResource(FMeshResource& Resource)
    {
        if (VertexBuffer == nullptr || IndexBuffer == nullptr)
        {
            Initialize();    
        }
    }

    void FMeshManager::Initialize()
    {
        FRHIBufferDesc Desc;
        Desc.bKeepInitialState = true;
        Desc.DebugName = "MeshManagerVertexBuffer";
        Desc.InitialState = EResourceStates::ShaderResource;
        Desc.Size = sizeof(FVertex) * 100'000;
        Desc.Usage.SetMultipleFlags(EBufferUsageFlags::StorageBuffer);
        
        VertexBuffer = GRenderContext->CreateBuffer(Desc);
        
        Desc.DebugName = "MeshManagerIndexBuffer";
        Desc.Size = sizeof(uint32) * 100'000;
        IndexBuffer = GRenderContext->CreateBuffer(Desc);
    }
}
