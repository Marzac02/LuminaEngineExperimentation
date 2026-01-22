#include "pch.h"
#include "Skeleton.h"

namespace Lumina
{
    void CSkeleton::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        
        if (!SkeletonResource)
        {
            SkeletonResource = MakeUnique<FSkeletonResource>();
        }
        
        Ar << *SkeletonResource;
    }

    void CSkeleton::ComputeBindPoseSkinningMatrices(TArray<glm::mat4, 255>& OutMatrices) const
    {
        TVector<glm::mat4> Transforms;
        Transforms.resize(SkeletonResource->GetNumBones());
    
        for (int i = 0; i < SkeletonResource->GetNumBones(); ++i)
        {
            const FSkeletonResource::FBoneInfo& Bone = SkeletonResource->GetBone(i);
            
            if (Bone.ParentIndex == INDEX_NONE)
            {
                Transforms[i] = Bone.LocalTransform;
            }
            else
            {
                Transforms[i] = Transforms[Bone.ParentIndex] * Bone.LocalTransform;
            }
        }
    
        for (int i = 0; i < SkeletonResource->GetNumBones(); ++i)
        {
            const FSkeletonResource::FBoneInfo& Bone = SkeletonResource->GetBone(i);
            OutMatrices[i] = Transforms[i] * Bone.InvBindMatrix;
        }
    }
}
