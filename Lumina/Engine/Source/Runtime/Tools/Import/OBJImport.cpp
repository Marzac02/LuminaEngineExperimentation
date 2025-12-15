#include "PCH.h"
#include "ImportHelpers.h"
#include <meshoptimizer.h>
#include <glm/glm.hpp>
#include "Renderer/MeshData.h"
#include <tinyobjloader/tiny_obj_loader.h>

#include "Paths/Paths.h"


namespace Lumina::Import::Mesh::OBJ
{
    bool ImportOBJ(FMeshImportData& OutData, const FMeshImportOptions& ImportOptions, FStringView FilePath)
    {
        tinyobj::ObjReaderConfig ReaderConfig;

        tinyobj::ObjReader Reader;

        if (!Reader.ParseFromFile(FilePath.data(), ReaderConfig))
        {
            if (!Reader.Error().empty())
            {
                LOG_ERROR("TinyObjReader Error: {}", Reader.Error());
            }

            return false;
        }

        if (!Reader.Warning().empty())
        {
            LOG_WARN("TinyObjReader Warning: {}", Reader.Warning());
        }
        
    
        const tinyobj::attrib_t& Attribute                  = Reader.GetAttrib();
        const std::vector<tinyobj::shape_t>& Shapes         = Reader.GetShapes();
        const std::vector<tinyobj::material_t>& Materials   = Reader.GetMaterials();

        OutData.Resources.clear();
        TUniquePtr<FMeshResource> MeshResource = MakeUniquePtr<FMeshResource>();
        MeshResource->Name = Paths::FileName(FilePath.data(), true);

        if (ImportOptions.bImportTextures)
        {
            for (const tinyobj::material_t& Material : Materials)
            {
                if (!Material.diffuse_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.diffuse_texname.c_str();
                    OutData.Textures.emplace(Image);
                }
        
                if (!Material.bump_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.bump_texname.c_str();
                    OutData.Textures.emplace(Image);
                }
        
                if (!Material.specular_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.specular_texname.c_str();
                    OutData.Textures.emplace(Image);
                }

                if (!Material.ambient_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.ambient_texname.c_str();
                    OutData.Textures.emplace(Image);
                }

                if (!Material.specular_highlight_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.specular_highlight_texname.c_str();
                    OutData.Textures.emplace(Image);
                }

                if (!Material.metallic_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.metallic_texname.c_str();
                    OutData.Textures.emplace(Image);
                }

                if (!Material.roughness_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.roughness_texname.c_str();
                    OutData.Textures.emplace(Image);
                }

                if (!Material.emissive_texname.empty())
                {
                    FMeshImportImage Image;
                    Image.ByteOffset = 0;
                    Image.RelativePath = Material.emissive_texname.c_str();
                    OutData.Textures.emplace(Image);
                }
            }
        }
        
        for (size_t ShapeIndex = 0; ShapeIndex < Shapes.size(); ++ShapeIndex)
        {
            size_t IndexOffset = 0;
            
            FGeometrySurface& Surface = MeshResource->GeometrySurfaces.emplace_back();
            Surface.ID = Shapes[ShapeIndex].name.c_str();
            Surface.MaterialIndex = 0;
            Surface.IndexCount = 0;
            Surface.StartIndex = static_cast<uint32>(MeshResource->Indices.size());
            
            for (size_t Face = 0; Face < Shapes[ShapeIndex].mesh.num_face_vertices.size(); ++Face)
            {
                Surface.MaterialIndex = (int16)Shapes[ShapeIndex].mesh.material_ids[Face];
                
                size_t NumFaceVerts = Shapes[ShapeIndex].mesh.num_face_vertices[Face];

                for (size_t V = 0; V < NumFaceVerts; ++V)
                {
                    tinyobj::index_t Index = Shapes[ShapeIndex].mesh.indices[IndexOffset + V];
    
                    MeshResource->Indices.push_back(static_cast<uint32>(MeshResource->Vertices.size()));
                    Surface.IndexCount++;
    
                    FVertex Vertex;
                    Vertex.Position.x = Attribute.vertices[3 * Index.vertex_index + 0];
                    Vertex.Position.y = Attribute.vertices[3 * Index.vertex_index + 1];
                    Vertex.Position.z = Attribute.vertices[3 * Index.vertex_index + 2];
    
                    if (Index.normal_index >= 0)
                    {
                        glm::vec3 Normal;
                        Normal.x = Attribute.normals[3 * Index.normal_index + 0];
                        Normal.y = Attribute.normals[3 * Index.normal_index + 1];
                        Normal.z = Attribute.normals[3 * Index.normal_index + 2];
                        Vertex.Normal = PackNormal(glm::normalize(Normal));
                    }

                    if (Index.texcoord_index >= 0)
                    {
                        Vertex.UV.x = (uint16)Attribute.texcoords[2 * Index.texcoord_index + 0];
                        Vertex.UV.y = (uint16)Attribute.texcoords[2 * Index.texcoord_index + 1];
                    }
    
                    MeshResource->Vertices.push_back(Vertex);
                }

                IndexOffset += NumFaceVerts;
            }
        }
        
        if (ImportOptions.bOptimize)
        {
            for (FGeometrySurface& Section : MeshResource->GeometrySurfaces)
            {
                meshopt_optimizeVertexCache(&MeshResource->Indices[Section.StartIndex], &MeshResource->Indices[Section.StartIndex], Section.IndexCount, MeshResource->Vertices.size());
                
                // Reorder indices for overdraw, balancing overdraw and vertex cache efficiency.
                // Allow up to 5% worse ACMR to get more reordering opportunities for overdraw.
                constexpr float Threshold = 1.05f;
                meshopt_optimizeOverdraw(&MeshResource->Indices[Section.StartIndex], &MeshResource->Indices[Section.StartIndex], Section.IndexCount, (float*)MeshResource->Vertices.data(), MeshResource->Vertices.size(), sizeof(FVertex), Threshold);
            }
        
            // Vertex fetch optimization should go last as it depends on the final index order
            meshopt_optimizeVertexFetch(MeshResource->Vertices.data(), MeshResource->Indices.data(), MeshResource->Indices.size(), MeshResource->Vertices.data(), MeshResource->Vertices.size(), sizeof(FVertex));
        }

        MeshResource->ShadowIndices = TVector<uint32>(MeshResource->Indices.size());
        meshopt_generateShadowIndexBuffer(MeshResource->ShadowIndices.data(), MeshResource->Indices.data(), MeshResource->Indices.size(), &MeshResource->Vertices[0].Position, MeshResource->Vertices.size(), sizeof(glm::vec4), sizeof(FVertex));
        meshopt_optimizeVertexCache(MeshResource->ShadowIndices.data(), MeshResource->ShadowIndices.data(), MeshResource->ShadowIndices.size(), MeshResource->Vertices.size());


        OutData.MeshStatistics.VertexFetchStatics.push_back(meshopt_analyzeVertexFetch(MeshResource->Indices.data(), MeshResource->Indices.size(), MeshResource->Vertices.size(), sizeof(FVertex)));
        OutData.MeshStatistics.OverdrawStatics.push_back(meshopt_analyzeOverdraw(MeshResource->Indices.data(), MeshResource->Indices.size(), (float*)MeshResource->Vertices.data(), MeshResource->Vertices.size(), sizeof(FVertex)));
        OutData.Resources.push_back(Move(MeshResource));
        
        return true;
    }
}

