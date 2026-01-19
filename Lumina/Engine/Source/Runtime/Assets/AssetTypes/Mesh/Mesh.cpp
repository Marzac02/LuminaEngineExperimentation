#include "pch.h"
#include "Mesh.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "assets/assettypes/material/materialinstance.h"
#include "Core/Object/Cast.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"
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
        
        for (SIZE_T i = 0; i < MeshResources->GetNumVertices(); ++i)
        {
            eastl::visit([&](auto& Vertex)
            {
                MeshResources->ExpandBounds(Vertex[i], BoundingBox);
            }, MeshResources->Vertices);
        }
    }

    void CMesh::GenerateGPUBuffers()
    {
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Transfer());
        CommandList->Open();
        
        uint64 VertexSize = MeshResources->GetVertexTypeSize() * MeshResources->GetNumVertices();

        FRHIBufferDesc VertexBufferDesc;
        VertexBufferDesc.Size = VertexSize;
        VertexBufferDesc.Usage.SetFlag(BUF_VertexBuffer);
        VertexBufferDesc.DebugName = GetName().ToString() + " Vertex Buffer";
        VertexBufferDesc.InitialState = EResourceStates::CopyDest;
        VertexBufferDesc.bKeepInitialState = true;
        MeshResources->MeshBuffers.VertexBuffer = GRenderContext->CreateBuffer(VertexBufferDesc);
        
        CommandList->WriteBuffer(MeshResources->MeshBuffers.VertexBuffer, MeshResources->GetVertexData(), VertexBufferDesc.Size);
        
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
        
        if (!MeshResources->bSkinnedMesh)
        {
            FVertexAttributeDesc VertexDesc[4];
            // Pos
            VertexDesc[0].SetElementStride(sizeof(FVertex));
            VertexDesc[0].SetOffset(offsetof(FVertex, Position));
            VertexDesc[0].Format = EFormat::RGB32_FLOAT;
        
            // Normal
            VertexDesc[1].SetElementStride(sizeof(FVertex));
            VertexDesc[1].SetOffset(offsetof(FVertex, Normal));
            VertexDesc[1].Format = EFormat::R32_UINT;
        
            // UV
            VertexDesc[2].SetElementStride(sizeof(FVertex));
            VertexDesc[2].SetOffset(offsetof(FVertex, UV));
            VertexDesc[2].Format = EFormat::RG16_UINT;
            
            // Color
            VertexDesc[3].SetElementStride(sizeof(FVertex));
            VertexDesc[3].SetOffset(offsetof(FVertex, Color));
            VertexDesc[3].Format = EFormat::RGBA8_UNORM;
            
            MeshResources->VertexLayout = GRenderContext->CreateInputLayout(VertexDesc, std::size(VertexDesc));
        }
        else
        {
            FVertexAttributeDesc VertexDesc[6];
            // Pos
            VertexDesc[0].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[0].SetOffset(offsetof(FSkinnedVertex, Position));
            VertexDesc[0].Format = EFormat::RGB32_FLOAT;
        
            // Normal
            VertexDesc[1].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[1].SetOffset(offsetof(FSkinnedVertex, Normal));
            VertexDesc[1].Format = EFormat::R32_UINT;
        
            // UV
            VertexDesc[2].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[2].SetOffset(offsetof(FSkinnedVertex, UV));
            VertexDesc[2].Format = EFormat::RG16_UINT;
            
            // Color
            VertexDesc[3].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[3].SetOffset(offsetof(FSkinnedVertex, Color));
            VertexDesc[3].Format = EFormat::RGBA8_UNORM;
            
            // Joint Indices
            VertexDesc[4].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[4].SetOffset(offsetof(FSkinnedVertex, JointIndices));
            VertexDesc[4].Format = EFormat::RGBA8_UINT;
            
            // Joint Weights
            VertexDesc[5].SetElementStride(sizeof(FSkinnedVertex));
            VertexDesc[5].SetOffset(offsetof(FSkinnedVertex, JointWeights));
            VertexDesc[5].Format = EFormat::RGBA8_UINT;
            
            MeshResources->VertexLayout = GRenderContext->CreateInputLayout(VertexDesc, std::size(VertexDesc));
        }
    }
}
