#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Core/Object/Object.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "ThumbnailManager.generated.h"

namespace Lumina
{
    struct FPackageThumbnail;
}

namespace Lumina
{
    REFLECT()
    class CThumbnailManager : public CObject
    {
        GENERATED_BODY()
    public:

        CThumbnailManager();

        void Initialize();
        
        static CThumbnailManager& Get();

        void AsyncLoadThumbnailsForPackage(const FName& Package);
        FPackageThumbnail* GetThumbnailForPackage(const FName& Package);
        
        PROPERTY(NotSerialized)
        TObjectPtr<CStaticMesh> CubeMesh;

        PROPERTY(NotSerialized)
        TObjectPtr<CStaticMesh> SphereMesh;

        PROPERTY(NotSerialized)
        TObjectPtr<CStaticMesh> PlaneMesh;
        
        
        FMutex ThumbnailLock;
        THashMap<FName, FPackageThumbnail*> Thumbnails;
        
    };
}
