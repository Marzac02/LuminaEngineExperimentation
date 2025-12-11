#include "pch.h"

#include "AssetRequest.h"

#include "Core/Object/Cast.h"
#include "Core/Object/Package/Package.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"

namespace Lumina
{
    bool FAssetRequest::Process()
    {
        FString FullPath = Paths::RemoveExtension(AssetPath);
        FullPath = Paths::ResolveVirtualPath(FullPath);
        
        CPackage* Package = CPackage::LoadPackage(FullPath.c_str());
        if (Package == nullptr)
        {
            LOG_INFO("Failed to load package at path: {}", FullPath);
            return false;
        }
        
        PendingObject = Package->LoadObject(RequestedGUID);
        if (PendingObject != nullptr)
        {
            if (PendingObject->HasAnyFlag(OF_NeedsPostLoad))
            {
                PendingObject->PostLoad();
                PendingObject->ClearFlags(OF_NeedsPostLoad);
            }
        }
    
        return true;
    }
}
 