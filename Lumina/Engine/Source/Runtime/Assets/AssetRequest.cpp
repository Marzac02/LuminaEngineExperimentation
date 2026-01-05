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
        CPackage* Package = CPackage::LoadPackage(AssetPath);
        if (Package == nullptr)
        {
            LOG_INFO("Failed to load package at path: {}", AssetPath);
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
 