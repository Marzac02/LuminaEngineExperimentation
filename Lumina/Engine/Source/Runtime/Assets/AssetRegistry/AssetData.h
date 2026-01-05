#pragma once

#include "Containers/Name.h"
#include "GUID/GUID.h"

namespace Lumina
{
    struct FAssetData
    {
        /** Globally unique ID for this asset */
        FGuid AssetGUID;

        /** Path of the asset. */
        FFixedString Path;

        /** Name of the asset without its package */
        FName AssetName;

        /** Path of the asset's class */
        FName AssetClass;
    };
}
