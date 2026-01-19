#include "pch.h"

#include <meshoptimizer.h>
#include <fastgltf/base64.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "ImportHelpers.h"
#include "Assets/AssetTypes/Mesh/Animation/Animation.h"
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
        
        for (const fastgltf::Animation& Animation : Asset.animations)
        {
            TUniquePtr<FAnimationClip> AnimClip = MakeUniquePtr<FAnimationClip>();
            AnimClip->Name = Animation.name.c_str();
            
            for (const fastgltf::AnimationChannel& Channel : Animation.channels)
            {
                FAnimationChannel AnimChannel;
                
                size_t NodeIndex = Channel.nodeIndex.value();
                const fastgltf::Node& Node = Asset.nodes[NodeIndex];
                AnimChannel.TargetBone = FName(Node.name.empty() ? ("Bone_" + eastl::to_string(NodeIndex)) : Node.name.c_str());
                
                if (Channel.path == fastgltf::AnimationPath::Translation)
                {
                    AnimChannel.TargetPath = FAnimationChannel::ETargetPath::Translation;
                }
                else if (Channel.path == fastgltf::AnimationPath::Rotation)
                {
                    AnimChannel.TargetPath = FAnimationChannel::ETargetPath::Rotation;
                }
                else if (Channel.path == fastgltf::AnimationPath::Scale)
                {
                    AnimChannel.TargetPath = FAnimationChannel::ETargetPath::Scale;
                }
                
                const auto& Sampler = Animation.samplers[Channel.samplerIndex];
                
                const auto& TimeAccessor = Asset.accessors[Sampler.inputAccessor];
                fastgltf::iterateAccessor<float>(Asset, TimeAccessor, [&](float time)
                {
                    AnimChannel.Timestamps.push_back(time);
                });
                
                const auto& ValueAccessor = Asset.accessors[Sampler.outputAccessor];
                if (Channel.path == fastgltf::AnimationPath::Translation || Channel.path == fastgltf::AnimationPath::Scale)
                {
                    fastgltf::iterateAccessor<glm::vec3>(Asset, ValueAccessor, [&](glm::vec3 Value)
                    {
                        if (Channel.path == fastgltf::AnimationPath::Translation)
                        {
                            AnimChannel.Translations.push_back(Value);
                        }
                        else
                        {
                            AnimChannel.Scales.push_back(Value);
                        }
                    });
                }
                else if (Channel.path == fastgltf::AnimationPath::Rotation)
                {
                    fastgltf::iterateAccessor<glm::vec4>(Asset, ValueAccessor, [&](glm::vec4 value)
                    {
                        AnimChannel.Rotations.push_back(glm::quat(value.w, value.x, value.y, value.z));
                    });
                }
                
                AnimClip->Channels.push_back(AnimChannel);
                AnimClip->Duration = glm::max(AnimClip->Duration, AnimChannel.Timestamps.back());
            }
            
            ImportData.Animations.push_back(Move(AnimClip));
        }
        
        for (const fastgltf::Skin& Skin : Asset.skins)
        {
            TUniquePtr<FSkeletonResource> NewSkeleton = MakeUniquePtr<FSkeletonResource>();
    
            if (Skin.name.empty())
            {
                NewSkeleton->Name = FName("Skeleton_" + eastl::to_string(ImportData.Skeletons.size()));
            }
            else
            {
                NewSkeleton->Name = FName(Skin.name.c_str());
            }
            
            NewSkeleton->Bones.reserve(Skin.joints.size());
            
            TVector<glm::mat4> InverseBindMatrices;
            if (Skin.inverseBindMatrices.has_value())
            {
                const fastgltf::Accessor& MatrixAccessor = Asset.accessors[Skin.inverseBindMatrices.value()];
                InverseBindMatrices.reserve(MatrixAccessor.count);
                
                fastgltf::iterateAccessor<glm::mat4>(Asset, MatrixAccessor, [&](const glm::mat4& matrix)
                {
                    InverseBindMatrices.push_back(matrix);
                });
            }
            
            THashMap<size_t, size_t> NodeToParent;
            for (size_t nodeIdx = 0; nodeIdx < Asset.nodes.size(); ++nodeIdx)
            {
                const fastgltf::Node& Node = Asset.nodes[nodeIdx];
                for (size_t ChildIdx : Node.children)
                {
                    NodeToParent[ChildIdx] = nodeIdx;
                }
            }
            
            for (size_t JointIdx = 0; JointIdx < Skin.joints.size(); ++JointIdx)
            {
                size_t NodeIdx = Skin.joints[JointIdx];
                const fastgltf::Node& BoneNode = Asset.nodes[NodeIdx];
                
                FSkeletonResource::FBoneInfo Bone;
                Bone.Name = FName(BoneNode.name.empty() ? ("Bone_" + eastl::to_string(NodeIdx)) : BoneNode.name.c_str());
                
                Bone.ParentIndex = -1;
                
                auto ParentIt = NodeToParent.find(NodeIdx);
                if (ParentIt != NodeToParent.end())
                {
                    size_t ParentNodeIdx = ParentIt->second;
        
                    for (size_t i = 0; i < Skin.joints.size(); ++i)
                    {
                        if (Skin.joints[i] == ParentNodeIdx)
                        {
                            Bone.ParentIndex = (int32)i;
                            break;
                        }
                    }
                }
                
                glm::mat4 LocalTransform(1.0f);
                if (auto* trs = std::get_if<fastgltf::TRS>(&BoneNode.transform))
                {
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2]));
                    glm::quat rotation(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);
                    glm::mat4 rotationMat = glm::mat4_cast(rotation);
                    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]));
                    LocalTransform = translation * rotationMat * scale;
                }
                else if (auto* mat = std::get_if<fastgltf::Node::TransformMatrix>(&BoneNode.transform))
                {
                    LocalTransform = glm::make_mat4(mat->data());
                }
                
                Bone.LocalTransform = LocalTransform;
                
                if (JointIdx < InverseBindMatrices.size())
                {
                    Bone.InvBindMatrix = InverseBindMatrices[JointIdx];
                }
                else
                {
                    Bone.InvBindMatrix = glm::mat4(1.0f);
                }
                
                NewSkeleton->Bones.push_back(Bone);
                NewSkeleton->BoneNameToIndex[Bone.Name] = (int32)JointIdx;
            }
            
            ImportData.Skeletons.push_back(Move(NewSkeleton));
        }
        
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
                    NewSurface.MaterialIndex = (int16)Primitive.materialIndex.value();
                }
                
                auto Joints = Primitive.findAttribute("JOINTS_0");
                auto Weights = Primitive.findAttribute("WEIGHTS_0");
                if (Joints != Primitive.attributes.end() && Weights != Primitive.attributes.end())
                {
                    NewResource->Vertices = TVector<FSkinnedVertex>();
                    NewResource->bSkinnedMesh = true;
                }
                else
                {
                    NewResource->Vertices = TVector<FVertex>();
                }
                
                SIZE_T InitialVert = NewResource->GetNumVertices();
            
                const fastgltf::Accessor& IndexAccessor = Asset.accessors[Primitive.indicesAccessor.value()];
                NewResource->Indices.reserve(NewResource->Indices.size() + IndexAccessor.count);
            
                fastgltf::iterateAccessor<uint32>(Asset, IndexAccessor, [&](uint32 Index)
                {
                    NewResource->Indices.push_back(Index);
                    NewSurface.IndexCount++;
                    IndexCount++;
                });
            
                const fastgltf::Accessor& PosAccessor = Asset.accessors[Primitive.findAttribute("POSITION")->second];
                eastl::visit([&](auto& Vector)
                {
                    Vector.resize(NewResource->GetNumVertices() + PosAccessor.count);
                }, NewResource->Vertices);
            
                // Initialize all vertices with defaults
                for (size_t i = InitialVert; i < NewResource->GetNumVertices(); ++i)
                {
                    NewResource->SetNormalAt(i, PackNormal(FViewVolume::UpAxis));
                    NewResource->SetUVAt(i, glm::u16vec2(0, 0));
                    NewResource->SetColorAt(i, 0xFFFFFFFF);
                }
                
                if (Joints != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::u8vec4>(Asset, Asset.accessors[Joints->second], [&](glm::u8vec4 v, size_t Index)
                    {
                        NewResource->SetJointIndicesAt(InitialVert + Index, v);
                        LOG_INFO("Joints {0}", glm::to_string(v));
                    });
                }
                
                if (Weights != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(Asset, Asset.accessors[Weights->second], [&](glm::vec4 v, size_t Index)
                    {
                        glm::u8vec4 WeightBytes = glm::u8vec4(v * 255.0f);
                        NewResource->SetJointWeightsAt(InitialVert + Index, WeightBytes);
                        LOG_INFO("Weights {0}", glm::to_string(WeightBytes));
                    });
                }
                
            
                // Load positions
                fastgltf::iterateAccessorWithIndex<glm::vec3>(Asset, PosAccessor, [&](glm::vec3 V, size_t Index)
                {
                    NewResource->SetPositionAt(InitialVert + Index, V);
                });
            
                // Load normals
                auto normals = Primitive.findAttribute("NORMAL");
                if (normals != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(Asset, Asset.accessors[normals->second], [&](glm::vec3 v, size_t index)
                    {
                        NewResource->SetNormalAt(InitialVert + index, PackNormal(glm::normalize(v)));
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
                        
                        glm::u16vec2 UV;
                        UV.x = (uint16)(glm::clamp(v.x, 0.0f, 1.0f) * 65535.0f);
                        UV.y = (uint16)(glm::clamp(v.y, 0.0f, 1.0f) * 65535.0f);
                        NewResource->SetUVAt(InitialVert + index, UV);
                    });
                }
            
                // Load vertex colors
                auto colors = Primitive.findAttribute("COLOR_0");
                if (colors != Primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(Asset, Asset.accessors[colors->second], [&](glm::vec4 v, size_t index)
                    {
                        NewResource->SetColorAt(InitialVert + index, PackColor(v));
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
