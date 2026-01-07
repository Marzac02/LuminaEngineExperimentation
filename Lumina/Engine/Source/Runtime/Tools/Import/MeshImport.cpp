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
            meshopt_optimizeVertexCache(&MeshResource.Indices[Section.StartIndex], &MeshResource.Indices[Section.StartIndex], Section.IndexCount, MeshResource.Vertices.size());
                
            // Reorder indices for overdraw, balancing overdraw and vertex cache efficiency.
            // Allow up to 5% worse ACMR to get more reordering opportunities for overdraw.
            constexpr float Threshold = 1.05f;
            meshopt_optimizeOverdraw(&MeshResource.Indices[Section.StartIndex], &MeshResource.Indices[Section.StartIndex], Section.IndexCount, reinterpret_cast<float*>(MeshResource.Vertices.data()), MeshResource.Vertices.size(), sizeof(FVertex), Threshold);
        }
        
        // Vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch(MeshResource.Vertices.data(), MeshResource.Indices.data(), MeshResource.Indices.size(), MeshResource.Vertices.data(), MeshResource.Vertices.size(), sizeof(FVertex));
    }

    void GenerateShadowBuffers(FMeshResource& MeshResource)
    {
        MeshResource.ShadowIndices = TVector<uint32>(MeshResource.Indices.size());
        
        meshopt_generateShadowIndexBuffer(MeshResource.ShadowIndices.data(), MeshResource.Indices.data(), MeshResource.Indices.size(), &MeshResource.Vertices[0].Position, MeshResource.Vertices.size(), sizeof(glm::vec4), sizeof(FVertex));
        meshopt_optimizeVertexCache(MeshResource.ShadowIndices.data(), MeshResource.ShadowIndices.data(), MeshResource.ShadowIndices.size(), MeshResource.Vertices.size());
    }

    void AnalyzeMeshStatistics(FMeshResource& MeshResource, FMeshStatistics& OutMeshStats)
    {
        TSpan<Byte> IndexBytes = AsBytes(MeshResource.Indices);
        TSpan<Byte> VertexBytes = AsBytes(MeshResource.Vertices);

        
        OutMeshStats.VertexFetchStatics.emplace_back(meshopt_analyzeVertexFetch(IndexBytes.data(), IndexBytes.size(), VertexBytes.size(), sizeof(FVertex)));
        OutMeshStats.OverdrawStatics.emplace_back(meshopt_analyzeOverdraw(IndexBytes.data(), IndexBytes.size(), reinterpret_cast<float*>(VertexBytes.data()), VertexBytes.size(), sizeof(FVertex)));
    }
}
