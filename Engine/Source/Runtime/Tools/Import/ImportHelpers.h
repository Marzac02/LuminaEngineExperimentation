#pragma once

#include <meshoptimizer.h>
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Core/Templates/Optional.h"
#include "Core/Utils/Expected.h"
#include "Memory/SmartPtr.h"
#include "Platform/Platform.h"
#include "Renderer/Format.h"
#include "Renderer/RenderResource.h"

namespace Lumina
{
    struct FAnimationResource;
    struct FMeshResource;
    struct FSkeletonResource;
    class IRenderContext;
    struct FVertex;
}

namespace Lumina::Import
{
    namespace Textures
    {
        struct FTextureImportResult
        {
            TVector<uint8> Pixels;
            glm::uvec2 Dimensions;
            EFormat Format;
        };
        
        /** Gets an image's raw pixel data */
        RUNTIME_API TOptional<FTextureImportResult> ImportTexture(FStringView RawFilePath, bool bFlipVertical = true);
    
        /** Creates a raw RHI Image */
        NODISCARD RUNTIME_API FRHIImageRef CreateTextureFromImport(IRenderContext* RenderContext, FStringView RawFilePath, bool bFlipVerticalOnLoad = true);
    }

    namespace Mesh
    {
        struct FMeshImportOptions
        {
            bool bOptimize          = true;
            bool bImportMaterials   = true;
            bool bImportTextures    = true;
            bool bImportMeshes      = true;
            bool bImportAnimations  = true;
            bool bImportSkeleton    = true;
            bool bFlipUVs           = false;
            float Scale             = 1.0f;
        };

        struct FMeshImportImage
        {
            FFixedString RelativePath;
            size_t ByteOffset;

            bool operator==(const FMeshImportImage& Other) const
            {
                return Other.RelativePath == RelativePath && Other.ByteOffset == ByteOffset;
            }
        };

        struct FMeshImportImageHasher
        {
            size_t operator()(const FMeshImportImage& Asset) const noexcept
            {
                size_t Seed = 0;
                Hash::HashCombine(Seed, Asset.RelativePath);
                Hash::HashCombine(Seed, Asset.ByteOffset);
                return Seed;
            }
        };
    
        struct FMeshImportImageEqual
        {
            bool operator()(const FMeshImportImage& A, const FMeshImportImage& B) const noexcept
            {
                return A.RelativePath == B.RelativePath && A.ByteOffset == B.ByteOffset;
            }
        };

        using FMeshImportTextureMap = THashSet<FMeshImportImage, FMeshImportImageHasher, FMeshImportImageEqual>;

        
        struct FMeshStatistics : INonCopyable
        {
            TVector<meshopt_OverdrawStatistics>         OverdrawStatics;
            TVector<meshopt_VertexFetchStatistics>      VertexFetchStatics;
        };

        struct FMeshImportData : INonCopyable
        {
            FMeshStatistics                             MeshStatistics;
            FMeshImportTextureMap                       Textures;
            TVector<TUniquePtr<FMeshResource>>          Resources;
            TVector<TUniquePtr<FAnimationResource>>     Animations;
            TVector<TUniquePtr<FSkeletonResource>>      Skeletons;
        };
        
        void OptimizeNewlyImportedMesh(FMeshResource& MeshResource);
        void GenerateShadowBuffers(FMeshResource& MeshResource);
        void AnalyzeMeshStatistics(FMeshResource& MeshResource, FMeshStatistics& OutMeshStats);
        

        namespace OBJ
        {
            NODISCARD RUNTIME_API TExpected<FMeshImportData, FString> ImportOBJ(const FMeshImportOptions& ImportOptions, FStringView FilePath);
        }
        
        namespace FBX
        {
            NODISCARD RUNTIME_API TExpected<FMeshImportData, FString> ImportFBX(const FMeshImportOptions& ImportOptions, FStringView FilePath);
        }

        
        namespace GLTF
        {
            NODISCARD RUNTIME_API TExpected<FMeshImportData, FString> ImportGLTF(const FMeshImportOptions& ImportOptions, FStringView FilePath);
        }
    }
    
}
