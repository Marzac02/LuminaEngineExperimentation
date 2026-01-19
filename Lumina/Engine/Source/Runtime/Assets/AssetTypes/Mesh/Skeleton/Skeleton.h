#pragma once

#include "Core/Object/Object.h"
#include "Renderer/MeshData.h"
#include "Core/Math/Transform.h"
#include "Memory/SmartPtr.h"
#include "Skeleton.generated.h"

namespace Lumina
{
    class CSkeletalMesh;
    REFLECT()
    class LUMINA_API CSkeleton : public CObject
    {
        friend class CMeshFactory;
        
        GENERATED_BODY()
        
    public:
        void Serialize(FArchive& Ar) override;

        bool IsAsset() const override { return true; }

        FSkeletonResource* GetSkeletonResource() const { return SkeletonResource.get(); }
        
        void ComputeBindPoseSkinningMatrices(TArray<glm::mat4, 255>& OutMatrices) const;
        
        PROPERTY(Editable, Category = "Preview")
        TObjectPtr<CSkeletalMesh> PreviewMesh;
        
    private:
        
        TUniquePtr<FSkeletonResource> SkeletonResource;
    };
}
