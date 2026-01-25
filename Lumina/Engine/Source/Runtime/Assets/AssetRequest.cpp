#include "pch.h"
#include "AssetRequest.h"
#include "Core/Object/Package/Package.h"

namespace Lumina
{
    bool FAssetRequest::Process()
    {
        if (CPackage* Package = CPackage::LoadPackage(AssetPath))
        {
            PendingObject = Package->LoadObject(RequestedGUID);
            return PendingObject != nullptr;
        }
        return false;
    }
}
 