#include "pch.h"

#include <meshoptimizer.h>
#include <fastgltf/base64.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>

#include "ImportHelpers.h"
#include "FileSystem/FileSystem.h"
#include "Memory/Memory.h"
#include "Paths/Paths.h"
#include "Renderer/MeshData.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderResource.h"
#include "Renderer/Vertex.h"

namespace Lumina::Import::Mesh::GLTF
{
    namespace
    {
        TExpected<fastgltf::Asset, FString> ExtractAsset(FStringView InPath)
        {
            std::filesystem::path FSPath(InPath.begin(), InPath.end());
        
            fastgltf::GltfDataBuffer Buffer;

            if (!Buffer.loadFromFile(FSPath))
            {
                return TUnexpected(std::format("Failed to load glTF model with path: {0}. Aborting import.", FSPath.string()).c_str());
            }

            fastgltf::GltfType SourceType = fastgltf::determineGltfFileType(&Buffer);

            if (SourceType == fastgltf::GltfType::Invalid)
            {
                return TUnexpected(std::format("Failed to determine glTF file type with path: {0}. Aborting import.", FSPath.string()).c_str());
            }

            constexpr fastgltf::Options options = fastgltf::Options::DontRequireValidAssetMember 
            | fastgltf::Options::LoadGLBBuffers 
            | fastgltf::Options::LoadExternalBuffers 
            | fastgltf::Options::GenerateMeshIndices 
            | fastgltf::Options::DecomposeNodeMatrices;

            fastgltf::Expected<fastgltf::Asset> Asset(fastgltf::Error::None);

            fastgltf::Parser Parser;
            if (SourceType == fastgltf::GltfType::glTF)
            {
                Asset = Parser.loadGltf(&Buffer, FSPath.parent_path(), options);
            }
            else if (SourceType == fastgltf::GltfType::GLB)
            {
                Asset = Parser.loadGltfBinary(&Buffer, FSPath.parent_path(), options);
            }

            if (const fastgltf::Error& Error = Asset.error(); Error != fastgltf::Error::None)
            {
                return TUnexpected(std::format("Failed to load asset source with path: {0}. [{1}]: {2} Aborting import.", FSPath.string(),
                fastgltf::getErrorName(Error), fastgltf::getErrorMessage(Error)).c_str());
            }

            return Move(Asset.get());
        }
    }
    

    TExpected<FMeshImportData, FString> ImportGLTF(const FMeshImportOptions& ImportOptions, FStringView FilePath)
    {
        TExpected<fastgltf::Asset, FString> ExpectedAsset = ExtractAsset(FilePath.data());
        if (ExpectedAsset.IsError())
        {
            return TUnexpected(ExpectedAsset.Error());
        }
        
        const fastgltf::Asset& Asset = ExpectedAsset.Value();
        
        FStringView Name = FileSystem::FileName(FilePath, true);
        
        FMeshImportData ImportData;
        ImportData.Resources.reserve(Asset.meshes.size());

        THashSet<FString> SeenMeshes;
        for (const fastgltf::Mesh& Mesh : Asset.meshes)
        {
            auto It = SeenMeshes.find(Mesh.name.c_str());
            if (It != SeenMeshes.end())
            {
                continue;
            }
            
            SeenMeshes.emplace(Mesh.name.c_str());
            
            TUniquePtr<FMeshResource> NewResource = MakeUniquePtr<FMeshResource>();
            NewResource->GeometrySurfaces.reserve(Mesh.primitives.size());
            
            FFixedString MeshName;
            if (Mesh.name.empty())
            {
                MeshName.append(Name.begin(), Name.end()).append_convert(eastl::to_string(ImportData.Resources.size()));
            }
            else
            {
                MeshName.append_convert(Mesh.name);
            }
            
            NewResource->Name = MeshName;

            SIZE_T IndexCount = 0;

            
            for (auto& Material : Asset.materials)
            {
                //FGLTFMaterial NewMaterial;
                //NewMaterial.Name = Material.name.c_str();
                //
                //OutData.Materials[OutData.Resources.size()].push_back(NewMaterial);
            }

            if (ImportOptions.bImportTextures)
            {
                for (auto& Image : Asset.images)
                {
                    auto& URI = std::get<fastgltf::sources::URI>(Image.data);
                    FMeshImportImage GLTFImage;
                    GLTFImage.ByteOffset = URI.fileByteOffset;
                    GLTFImage.RelativePath = URI.uri.c_str();
                    ImportData.Textures.emplace(GLTFImage);
                }
            }
            
            for (auto& Primitive : Mesh.primitives)
            {
                FGeometrySurface NewSurface;
                NewSurface.StartIndex = (uint32)IndexCount;
                
                FFixedString PrimitiveName;
                if (Mesh.name.empty())
                {
                    PrimitiveName.append(Name.begin(), Name.end()).append_convert(eastl::to_string(NewResource->GetNumSurfaces()));
                }
                else
                {
                    PrimitiveName.append_convert(Mesh.name);
                }
            
                
                NewSurface.ID = PrimitiveName;
                
                if (Primitive.materialIndex.has_value())
                {
                    NewSurface.MaterialIndex = (int64)Primitive.materialIndex.value();
                }
                
                SIZE_T InitialVert = NewResource->Vertices.size();
            
                const fastgltf::Accessor& IndexAccessor = Asset.accessors[Primitive.indicesAccessor.value()];
                NewResource->Indices.reserve(NewResource->Indices.size() + IndexAccessor.count);
            
                fastgltf::iterateAccessor<uint32>(Asset, IndexAccessor, [&](uint32 Index)
                {
                    NewResource->Indices.push_back(Index);
                    NewSurface.IndexCount++;
                    IndexCount++;
                });
            
                const fastgltf::Accessor& PosAccessor = Asset.accessors[Primitive.findAttribute("POSITION")->second];
                NewResource->Vertices.resize(NewResource->Vertices.size() + PosAccessor.count);
            
                // Initialize all vertices with defaults
                for (size_t i = InitialVert; i < NewResource->Vertices.size(); ++i)
                {
                    NewResource->Vertices[i].Normal = PackNormal(FViewVolume::UpAxis);
                    NewResource->Vertices[i].UV = glm::u16vec2(0, 0);
                    NewResource->Vertices[i].Color = 0xFFFFFFFF;
                }
            
                // Load positions
                fastgltf::iterateAccessorWithIndex<glm::vec3>(Asset, PosAccessor, [&](glm::vec3 V, size_t Index)
                {
                    NewResource->Vertices[InitialVert + Index].Position = V;
                });
            
                // Load normals
                auto normals = Primitive.findAttribute("NORMAL");
                if (normals != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(Asset, Asset.accessors[normals->second], [&](glm::vec3 v, size_t index)
                    {
                        NewResource->Vertices[InitialVert + index].Normal = PackNormal(glm::normalize(v));
                    });
                }
            
                // Load UVs
                auto uv = Primitive.findAttribute("TEXCOORD_0");
                if (uv != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(Asset, Asset.accessors[uv->second], [&](glm::vec2 v, size_t index)
                    {
                        // Convert float UVs to uint16 (0.0-1.0 -> 0-65535)
                        if (ImportOptions.bFlipUVs)
                        {
                            v.y = 1.0f - v.y;
                        }
                        NewResource->Vertices[InitialVert + index].UV.x = (uint16)(glm::clamp(v.x, 0.0f, 1.0f) * 65535.0f);
                        NewResource->Vertices[InitialVert + index].UV.y = (uint16)(glm::clamp(v.y, 0.0f, 1.0f) * 65535.0f);
                    });
                }
            
                // Load vertex colors
                auto colors = Primitive.findAttribute("COLOR_0");
                if (colors != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(Asset, Asset.accessors[colors->second], [&](glm::vec4 v, size_t index)
                    {
                        NewResource->Vertices[InitialVert + index].Color = PackColor(v);
                    });
                }
                
                NewResource->GeometrySurfaces.push_back(NewSurface);
            }
            
            if (ImportOptions.bOptimize)
            {
                OptimizeNewlyImportedMesh(*NewResource);
            }
        
            GenerateShadowBuffers(*NewResource);
            AnalyzeMeshStatistics(*NewResource, ImportData.MeshStatistics);
            
            ImportData.Resources.push_back(eastl::move(NewResource));
        }

        return Move(ImportData);
    }
}
