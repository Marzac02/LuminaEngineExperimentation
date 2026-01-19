#pragma once

#include "Core/Math/AABB.h"
#include "Core/Object/Object.h"
#include "Animation.generated.h"
#include "Memory/SmartPtr.h"

namespace Lumina
{
    class CSkeleton;
    struct FSkeletonResource;

    struct FAnimationChannel
    {
        enum class ETargetPath : uint8
        {
            Translation,
            Rotation,
            Scale,
            Weights
        };
    
        FName TargetBone; 
        ETargetPath TargetPath;
        TVector<float> Timestamps;
        TVector<glm::vec3> Translations;
        TVector<glm::quat> Rotations;
        TVector<glm::vec3> Scales;
        
        friend FArchive& operator << (FArchive& Ar, FAnimationChannel& Data)
        {
            Ar << Data.TargetBone;
            Ar << Data.TargetPath;
            Ar << Data.Timestamps;
            Ar << Data.Translations;
            Ar << Data.Rotations;
            Ar << Data.Scales;
            
            return Ar;
        }
        
    };
        
    struct FAnimationClip
    {
        FName Name;
        float Duration;
        TVector<FAnimationChannel> Channels;
        
        friend FArchive& operator << (FArchive& Ar, FAnimationClip& Data)
        {
            Ar << Data.Name;
            Ar << Data.Duration;
            Ar << Data.Channels;
            
            return Ar;
        }
        
    };
    
    
    REFLECT()
    class LUMINA_API CAnimation : public CObject
    {
        GENERATED_BODY()
        
        friend class CMeshFactory;
        
    public:
        
        void Serialize(FArchive& Ar) override;
        
        bool IsAsset() const override { return true; }
        
        void SamplePose(float Time, FSkeletonResource* Skeleton, TArray<glm::mat4, 255>& OutBoneTransforms);
        
        float GetDuration() const { return AnimationClip->Duration; }
        
        PROPERTY(Editable, Category = "Skeleton")
        TObjectPtr<CSkeleton> Skeleton;
        
    private:
        
        glm::vec3 SampleVec3(const TVector<float>& Times, const TVector<glm::vec3>& Values, float Time);
        glm::quat SampleQuat(const TVector<float>& Times, const TVector<glm::quat>& Values, float Time);

        
        
        TUniquePtr<FAnimationClip> AnimationClip;
    };
}
