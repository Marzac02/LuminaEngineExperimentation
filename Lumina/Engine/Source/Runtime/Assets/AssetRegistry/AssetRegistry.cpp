#include "pch.h"
#include "AssetRegistry.h"

#include "Core/Object/Package/Package.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"
#include "TaskSystem/TaskSystem.h"

namespace Lumina
{
    FAssetRegistry& FAssetRegistry::Get()
    {
        static FAssetRegistry Registry;
        return Registry;
    }

    void FAssetRegistry::ProjectLoaded()
    {
        RunInitialDiscovery();
    }
    
    void FAssetRegistry::RunInitialDiscovery()
    {
        LUMINA_PROFILE_SCOPE();

        ClearAssets();
        
        TFixedVector<FFixedString, 256> PackagePaths;
        for (const auto& [ID, Path] : Paths::GetMountedPaths())
        {
            for (const auto& Directory : std::filesystem::recursive_directory_iterator(Path.c_str()))
            {
                if (!Directory.is_directory() && Directory.path().extension() == ".lasset")
                {
                    PackagePaths.push_back(Directory.path().generic_string().c_str());
                }
            }
        }
        
        uint32 NumPackages = (uint32)PackagePaths.size();
        Task::AsyncTask(NumPackages, NumPackages, [this, PackagePaths = std::move(PackagePaths)] (uint32 Start, uint32 End, uint32)
        {
            for (uint32 i = Start; i < End; ++i)
            {
                const FFixedString& PathString = PackagePaths[i];
                ProcessPackagePath(PathString);
            }
        
            if (End == PackagePaths.size())
            {
                OnInitialDiscoveryCompleted();
            }
        });
    }

    void FAssetRegistry::OnInitialDiscoveryCompleted()
    {
        LOG_INFO("Asset Registry Finished Initial Discovery: Num [{}]", Assets.size());
    }

    void FAssetRegistry::AssetCreated(CObject* Asset)
    {
        FString FilePath = Paths::ResolveVirtualPath(Asset->GetPackage()->GetName().ToString()) + ".lasset";
        
        TSharedPtr<FAssetData> AssetData = MakeSharedPtr<FAssetData>();
        AssetData->AssetClass   = Asset->GetClass()->GetName();
        AssetData->AssetGUID    = Asset->GetGUID();
        AssetData->AssetName    = Asset->GetName();
        AssetData->FilePath     = Move(FilePath);
        AssetData->PackageName  = Asset->GetPackage()->GetName();

        FScopeLock Lock(AssetsMutex);
        Assets.emplace(Move(AssetData));

        GetOnAssetRegistryUpdated().Broadcast();
    }

    void FAssetRegistry::AssetDeleted(const FGuid& GUID)
    {
        FScopeLock Lock(AssetsMutex);

        auto It = Assets.find_as(GUID, FGuidHash(), FAssetDataGuidEqual());
        LUM_ASSERT(It != Assets.end())

        Assets.erase(It);
        
        GetOnAssetRegistryUpdated().Broadcast();
    }

    void FAssetRegistry::AssetRenamed(const FString& OldPath, const FString& NewPath)
    {
        FScopeLock Lock(AssetsMutex);

        auto It = eastl::find_if(Assets.begin(), Assets.end(), [&OldPath](const TSharedPtr<FAssetData>& Asset)
        {
            return Asset->FilePath == OldPath;
        });

        LUM_ASSERT(It != Assets.end())

        TSharedPtr<FAssetData> Data = *It;
        Data->PackageName = Paths::ConvertToVirtualPath(NewPath);
        Data->FilePath = NewPath;

        GetOnAssetRegistryUpdated().Broadcast();
    }

    void FAssetRegistry::AssetSaved(CObject* Asset)
    {
        FScopeLock Lock(AssetsMutex);
        
        GetOnAssetRegistryUpdated().Broadcast();
    }

    LUMINA_DISABLE_OPTIMIZATION
    void FAssetRegistry::ProcessPackagePath(FStringView Path)
    {
        TVector<uint8> PackageBlob;
        if (!FileHelper::LoadFileToArray(PackageBlob, Path))
        {
            LOG_ERROR("Failed to load package file at path {}", Path);

            return;
        }

        FName PackageFileName = Paths::FileName(Path.data(), true);


        FPackageHeader Header;
        FMemoryReader Reader(PackageBlob);
        Reader << Header;

        Reader.Seek(Header.ExportTableOffset);
        
        TVector<FObjectExport> Exports;
        Reader << Exports;

        TOptional<FObjectExport> Export;
        for (const FObjectExport& E : Exports)
        {
           if (E.ObjectName == PackageFileName)
           {
               Export = E;
               break;
           }
        }

        if (!Export.has_value())
        {
            LOG_ERROR("No primary asset found in package");
            return;
        }

        TSharedPtr<FAssetData> AssetData = MakeSharedPtr<FAssetData>();
        AssetData->AssetClass   = Export->ClassName;
        AssetData->AssetGUID    = Export->ObjectGUID;
        AssetData->AssetName    = Export->ObjectName;
        AssetData->FilePath     = Path;
        AssetData->PackageName  = Paths::ConvertToVirtualPath(Path.data());

        FScopeLock Lock(AssetsMutex);
        Assets.emplace(Move(AssetData));
    }
    LUMINA_ENABLE_OPTIMIZATION
    void FAssetRegistry::ClearAssets()
    {
        FScopeLock Lock(AssetsMutex);

        Assets.clear();

        BroadcastRegistryUpdate();
    }

    void FAssetRegistry::BroadcastRegistryUpdate()
    {
        OnAssetRegistryUpdated.Broadcast();
    }
    
    bool FClassPredicate::Evaluate(const FAssetData& Asset) const
    {
        if (!bIncludeDerived)
        {
            return Asset.AssetClass == ClassName;
        }

        const CClass* A = FindObject<CClass>(ClassName);
        const CClass* B = FindObject<CClass>(Asset.AssetClass);
        LUM_ASSERT(A && B)
        
        return B->IsChildOf(A);
    }
}
