#include "ThumbnailManager.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "Paths/Paths.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"

#include "TaskSystem/TaskSystem.h"
#include "World/Scene/RenderScene/SceneMeshes.h"


namespace Lumina
{

    static CThumbnailManager* ThumbnailManagerSingleton = nullptr;

    CThumbnailManager::CThumbnailManager()
    {
    }

    void CThumbnailManager::Initialize()
    {
        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GenerateCube(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "CubeMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            CubeMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailCubeMesh", FGuid::New(), OF_Transient);
            CubeMesh->Materials.resize(1);
            CubeMesh->SetMeshResource(Move(Resource));
        }

        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GenerateSphere(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "SphereMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            SphereMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailSphereMesh", FGuid::New(), OF_Transient);
            SphereMesh->Materials.resize(1);
            SphereMesh->SetMeshResource(Move(Resource));
        }

        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GeneratePlane(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "PlaneMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            PlaneMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailPlaneMesh", FGuid::New(), OF_Transient);
            PlaneMesh->Materials.resize(1);
            PlaneMesh->SetMeshResource(Move(Resource));
        }
    }

    CThumbnailManager& CThumbnailManager::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            ThumbnailManagerSingleton = NewObject<CThumbnailManager>();
            ThumbnailManagerSingleton->Initialize();
        });

        return *ThumbnailManagerSingleton;
    }

    void CThumbnailManager::TryLoadThumbnailsForPackage(const FString& PackagePath)
    {
        FString ActualPackagePath = PackagePath;
        //if (!Paths::HasExtension(ActualPackagePath, "lasset"))
        //{
        //    Paths::AddPackageExtension(ActualPackagePath);
        //}
        if (CPackage* Package = CPackage::LoadPackage(ActualPackagePath))
        {
            TSharedPtr<FPackageThumbnail> Thumbnail = Package->GetPackageThumbnail();
        
            if (!Thumbnail->bDirty || Thumbnail->ImageData.empty())
            {
                return;
            }
            
            FRHIImageDesc ImageDesc;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.Extent = {256, 256};
            ImageDesc.Format = EFormat::RGBA8_UNORM;
            ImageDesc.Flags.SetFlag(EImageCreateFlags::ShaderResource);
            FRHIImageRef Image = GRenderContext->CreateImage(ImageDesc);
        
            FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Compute());
            CommandList->Open();
            CommandList->BeginTrackingImageState(Image, AllSubresources, EResourceStates::CopyDest);
            
            const uint8 BytesPerPixel = RHI::Format::BytesPerBlock(ImageDesc.Format);
            const uint32 RowBytes = ImageDesc.Extent.y * BytesPerPixel;
            
            TVector<uint8> FlippedData(Thumbnail->ImageData.size());
            Task::ParallelFor(ImageDesc.Extent.y, [&](uint32 Y)
            {
                const uint32 FlippedY = 255 - Y;
                Memory::Memcpy(FlippedData.data() + (FlippedY * RowBytes), Thumbnail->ImageData.data() + (Y * RowBytes), RowBytes);
            });

            const uint32 RowPitch = RowBytes;
            constexpr uint32 DepthPitch = 0;
        
            CommandList->WriteImage(Image, 0, 0, FlippedData.data(), RowPitch, DepthPitch);

            CommandList->SetPermanentImageState(Image, EResourceStates::ShaderResource);
            CommandList->CommitBarriers();
            
            CommandList->Close();
        
            GRenderContext->ExecuteCommandList(CommandList, ECommandQueue::Compute);
        
            Thumbnail->LoadedImage = Image;
            Thumbnail->bDirty = false;
        }
    }

    FPackageThumbnail* CThumbnailManager::GetThumbnailForPackage(CPackage* Package)
    {
        return Package->GetPackageThumbnail().get();
    }
}
