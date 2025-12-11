#pragma once

#include "Containers/Name.h"
#include "GUID/GUID.h"

namespace Lumina
{
    struct FAssetData
    {
        /** Globally unique ID for this asset */
        FGuid AssetGUID;

        /** Path of this asset on disk */
        FString FilePath;

        /** Name of the asset without its package */
        FName AssetName;
        
        /** Name of the package owning this asset */
        FName PackageName;

        /** Path of the asset's class */
        FName AssetClass;
    };
}
