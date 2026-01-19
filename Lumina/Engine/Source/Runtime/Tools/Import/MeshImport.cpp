#include "PCH.h"
#include "ImportHelpers.h"
#include "Core/Templates/AsBytes.h"
#include "Renderer/MeshData.h"

namespace Lumina::Import::Mesh
{
    void OptimizeNewlyImportedMesh(FMeshResource& MeshResource)
    {
        for (FGeometrySurface& Section : MeshResource.GeometrySurfaces)
        {
            meshopt_optimizeVertexCache(&MeshResource.Indices[Section.StartIndex], &MeshResource.Indices[Section.StartIndex], Section.IndexCount, MeshResource.GetNumVertices());
                
            // Reorder indices for overdraw, balancing overdraw and vertex cache efficiency.
            // Allow up to 5% worse ACMR to get more reordering opportunities for overdraw.
            constexpr float Threshold = 1.05f;
            meshopt_optimizeOverdraw(&MeshResource.Indices[Section.StartIndex], &MeshResource.Indices[Section.StartIndex], Section.IndexCount, static_cast<float*>(MeshResource.GetVertexData()), MeshResource.GetNumVertices(), MeshResource.GetVertexTypeSize(), Threshold);
        }
        
        // Vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch(MeshResource.GetVertexData(), MeshResource.Indices.data(), MeshResource.Indices.size(), MeshResource.GetVertexData(), MeshResource.GetNumVertices(), MeshResource.GetVertexTypeSize());
    }

    void GenerateShadowBuffers(FMeshResource& MeshResource)
    {
        MeshResource.ShadowIndices = TVector<uint32>(MeshResource.Indices.size());
        meshopt_generateShadowIndexBuffer(MeshResource.ShadowIndices.data(), MeshResource.Indices.data(), MeshResource.Indices.size(), MeshResource.GetVertexData(), MeshResource.GetNumVertices(), sizeof(glm::vec4), MeshResource.GetVertexTypeSize());
        meshopt_optimizeVertexCache(MeshResource.ShadowIndices.data(), MeshResource.ShadowIndices.data(), MeshResource.ShadowIndices.size(), MeshResource.GetNumVertices());
    }

    void AnalyzeMeshStatistics(FMeshResource& MeshResource, FMeshStatistics& OutMeshStats)
    {
        OutMeshStats.VertexFetchStatics.emplace_back(meshopt_analyzeVertexFetch(MeshResource.Indices.data(), MeshResource.Indices.size(), MeshResource.GetNumVertices(), MeshResource.GetVertexTypeSize()));
        OutMeshStats.OverdrawStatics.emplace_back(meshopt_analyzeOverdraw(MeshResource.Indices.data(), MeshResource.Indices.size(), static_cast<float*>(MeshResource.GetVertexData()), MeshResource.GetNumVertices(), MeshResource.GetVertexTypeSize()));
    }
}
