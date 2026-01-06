#include "pch.h"
#include "Mesh.h"

#include "Renderer/RenderContext.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "assets/assettypes/material/materialinstance.h"
#include "Core/Object/Cast.h"
#include "Renderer/RHIGlobals.h"
#include "Renderer/TypedBuffer.h"
#include "Renderer/Vertex.h"


namespace Lumina
{
    void CMesh::Serialize(FArchive& Ar)
    {
        Super::Serialize(Ar);

        if (!MeshResources)
        {
            MeshResources = MakeUniquePtr<FMeshResource>();
        }
        
        Ar << *MeshResources;
    }

    void CMesh::Serialize(IStructuredArchive::FSlot Slot)
    {
        CObject::Serialize(Slot);
    }

    void CMesh::PostLoad()
    {
        GenerateBoundingBox();
        GenerateGPUBuffers();
    }

    CMaterialInterface* CMesh::GetMaterialAtSlot(SIZE_T Slot) const
    {
        return Materials.empty() ? nullptr : Materials[Slot].Get();
    }

    void CMesh::SetMaterialAtSlot(SIZE_T Slot, CMaterialInterface* NewMaterial)
    {
        if (Materials.size() <= Slot)
        {
            Materials.push_back(NewMaterial);
        }
        else
        {
            Materials[Slot] = NewMaterial;
        }  
    }

    void CMesh::SetMeshResource(TUniquePtr<FMeshResource>&& NewResource)
    {
        MeshResources = eastl::move(NewResource);
        GenerateBoundingBox();
        GenerateGPUBuffers();
    }

    bool CMesh::IsReadyForRender() const
    {
        LUMINA_PROFILE_SCOPE();

        if (HasAnyFlag(OF_NeedsLoad))
        {
            return false;
        }

        for (CMaterialInterface* Material : Materials)
        {
            if (Material == nullptr)
            {
                return false;
            }

            if (Material->IsReadyForRender() == false)
            {
                return false;
            }
        }

        return !Materials.empty();
    }

    void CMesh::GenerateBoundingBox()
    {
        BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
        BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        
        for (SIZE_T i = 0; i < MeshResources->Vertices.size(); ++i)
        {
            const FVertex& Vertex = MeshResources->Vertices[i];
            
            BoundingBox.Min.x = glm::min(Vertex.Position.x, BoundingBox.Min.x);
            BoundingBox.Min.y = glm::min(Vertex.Position.y, BoundingBox.Min.y);
            BoundingBox.Min.z = glm::min(Vertex.Position.z, BoundingBox.Min.z);
        
            BoundingBox.Max.x = glm::max(Vertex.Position.x, BoundingBox.Max.x);
            BoundingBox.Max.y = glm::max(Vertex.Position.y, BoundingBox.Max.y);
            BoundingBox.Max.z = glm::max(Vertex.Position.z, BoundingBox.Max.z);
        }
    }

    void CMesh::GenerateGPUBuffers()
    {
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Transfer());
        CommandList->Open();
        
        uint64 VertexSize = sizeof(FVertex) * MeshResources->Vertices.size();

        FRHIBufferDesc VertexBufferDesc;
        VertexBufferDesc.Size = VertexSize;
        VertexBufferDesc.Usage.SetFlag(BUF_VertexBuffer);
        VertexBufferDesc.DebugName = GetName().ToString() + " Vertex Buffer";
        VertexBufferDesc.InitialState = EResourceStates::CopyDest;
        VertexBufferDesc.bKeepInitialState = true;
        MeshResources->MeshBuffers.VertexBuffer = GRenderContext->CreateBuffer(VertexBufferDesc);
        
        CommandList->WriteBuffer(MeshResources->MeshBuffers.VertexBuffer, MeshResources->Vertices.data(), VertexBufferDesc.Size);
        
        uint64 IndexSize = sizeof(uint32) * MeshResources->Indices.size();
        
        FRHIBufferDesc IndexBufferDesc;
        IndexBufferDesc.Size = IndexSize;
        IndexBufferDesc.InitialState = EResourceStates::CopyDest;
        IndexBufferDesc.bKeepInitialState = true;
        IndexBufferDesc.Usage.SetFlag(BUF_IndexBuffer);
        IndexBufferDesc.DebugName = GetName().ToString() + " Index Buffer";
        MeshResources->MeshBuffers.IndexBuffer = GRenderContext->CreateBuffer(IndexBufferDesc);
        
        CommandList->WriteBuffer(MeshResources->MeshBuffers.IndexBuffer, MeshResources->Indices.data(), IndexBufferDesc.Size);
        
        MeshResources->MeshBuffers.ShadowIndexBuffer = MeshResources->MeshBuffers.IndexBuffer;
        
        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList, ECommandQueue::Transfer);
        
        
        FRHICommandListRef GCommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        GCommandList->Open();
        GCommandList->SetPermanentBufferState(MeshResources->MeshBuffers.VertexBuffer, EResourceStates::VertexBuffer);
        GCommandList->SetPermanentBufferState(MeshResources->MeshBuffers.IndexBuffer, EResourceStates::IndexBuffer);
        GCommandList->Close();
        
        GRenderContext->ExecuteCommandList(GCommandList, ECommandQueue::Graphics);

    }
}
