#include "pch.h"
#include "ObjectRename.h"

#include <filesystem>

#include "Assets/AssetManager/AssetManager.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Engine/Engine.h"
#include "Package/Package.h"
#include "Paths/Paths.h"

namespace Lumina::ObjectRename
{
    
    EObjectRenameResult RenameObject(const FString& OldPath, FString NewPath)
    {
        try
        {
            FName VirtualName = Paths::ConvertToVirtualPath(NewPath);
            if (FindObjectFast(nullptr, nullptr, VirtualName))
            {
                LOG_ERROR("Destination path already exists: {0}", NewPath);
                return EObjectRenameResult::Exists;
            }
            
            if (!Paths::HasExtension(NewPath, "lasset"))
            {
                NewPath += ".lasset";
            }
            
            if (CPackage* OldPackage = CPackage::LoadPackage(OldPath))
            {
                // Make sure nothing is currently loading before executing this rename.
                GEngine->GetEngineSubsystem<FAssetManager>()->FlushAsyncLoading();
                
                /** We need all objects to be loaded to rename a package */
                LUM_ASSERT(OldPackage->FullyLoad())

                bool bCreateRedirectors = false;
                
                FAssetRegistry* AssetRegistry = GEngine->GetEngineSubsystem<FAssetRegistry>();
                const THashSet<FName>& Dependencies = AssetRegistry->GetDependencies(OldPackage->GetName());

                for (const FName& Dependency : Dependencies)
                {
                    FString FullPath = Paths::ResolveVirtualPath(Dependency.ToString());
                    CPackage::LoadPackage(FullPath);
                }
                
                FString OldAssetName = Paths::FileName(OldPath, true);
                FString NewAssetName = Paths::FileName(NewPath, true);

                CPackage* NewPackage = CPackage::CreatePackage(NewPath);
                
                TVector<TObjectPtr<CObject>> Objects;
                GetObjectsWithPackage(OldPackage, Objects);

                for (CObject* Object : Objects)
                {
                    if (Object->IsAsset())
                    {
                        Object->Rename(NewAssetName, NewPackage, bCreateRedirectors);
                        AssetRegistry->AssetRenamed(Object, OldPath);
                    }
                    else
                    {
                        Object->Rename(Object->GetName(), NewPackage, false);
                    }
                }

                CPackage::SavePackage(NewPackage, NewPath);

                if (bCreateRedirectors)
                {
                    CPackage::SavePackage(OldPackage, OldPath);
                }
                else
                {
                    CPackage::DestroyPackage(OldPath);
                }
            }
            
            return EObjectRenameResult::Success;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR("Failed to rename file: {0}", e.what());
            return EObjectRenameResult::Failure;
        }
    }
    
}
