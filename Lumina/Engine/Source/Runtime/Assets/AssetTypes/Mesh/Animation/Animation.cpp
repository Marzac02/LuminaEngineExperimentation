#include "pch.h"
#include "Animation.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "Renderer/MeshData.h"


namespace Lumina
{
    void CAnimation::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        
        if (!AnimationClip)
        {
            AnimationClip = MakeUniquePtr<FAnimationClip>();
        }
        
        Ar << *AnimationClip;
    }
    
    void CAnimation::SamplePose(float Time, FSkeletonResource* Skeleton, TArray<glm::mat4, 255>& OutBoneTransforms)
    {
        LUMINA_PROFILE_SCOPE();
        
        const int32 NumBones = Skeleton->GetNumBones();
    
        TVector<glm::vec3> Translations(NumBones);
        TVector<glm::quat> Rotations(NumBones);
        TVector<glm::vec3> Scales(NumBones);
    
        for (const FAnimationChannel& Channel : AnimationClip->Channels)
        {
            int i = Skeleton->FindBoneIndex(Channel.TargetBone);
            if (i < 0 || i >= NumBones)
            {
                continue;
            }

            switch (Channel.TargetPath)
            {
            case FAnimationChannel::ETargetPath::Translation:
                Translations[i] = SampleVec3(Channel.Timestamps, Channel.Translations, Time);
                break;
    
            case FAnimationChannel::ETargetPath::Rotation:
                Rotations[i] = SampleQuat(Channel.Timestamps, Channel.Rotations, Time);
                break;
    
            case FAnimationChannel::ETargetPath::Scale:
                Scales[i] = SampleVec3(Channel.Timestamps, Channel.Scales, Time);
                break;
    
            default:
                break;
            }
        }
    
        TVector<glm::mat4> Local(NumBones);
        for (int i = 0; i < NumBones; ++i)
        {
            Local[i] =
                glm::translate(glm::mat4(1.0f), Translations[i]) *
                glm::mat4_cast(Rotations[i]) *
                glm::scale(glm::mat4(1.0f), Scales[i]);
        }
    
        TVector<glm::mat4> Global(NumBones);
        for (int i = 0; i < NumBones; ++i)
        {
            const auto& Bone = Skeleton->GetBone(i);
            if (Bone.ParentIndex == INDEX_NONE)
            {
                Global[i] = Local[i];
            }
            else
            {
                Global[i] = Global[Bone.ParentIndex] * Local[i];
            }
        }
    
        for (int i = 0; i < NumBones; ++i)
        {
            const auto& Bone = Skeleton->GetBone(i);
            OutBoneTransforms[i] = Global[i] * Bone.InvBindMatrix;
        }
    }

    glm::vec3 CAnimation::SampleVec3(const TVector<float>& Times, const TVector<glm::vec3>& Values, float Time)
    {
        if (Times.empty() || Values.empty())
        {
            return glm::vec3(0.0f);
        }
        
        if (Times.size() == 1)
        {
            return Values[0];
        }
        
        if (Time <= Times[0])
        {
            return Values[0];
        }
        
        if (Time >= Times[Times.size() - 1])
        {
            return Values[Values.size() - 1];
        }
        
        for (size_t i = 0; i < Times.size() - 1; ++i)
        {
            if (Time >= Times[i] && Time < Times[i + 1])
            {
                float DeltaTime = Times[i + 1] - Times[i];
                float BlendFactor = (Time - Times[i]) / DeltaTime;
                
                return glm::mix(Values[i], Values[i + 1], BlendFactor);
            }
        }
        
        return Values[Values.size() - 1];
    }

    glm::quat CAnimation::SampleQuat(const TVector<float>& Times, const TVector<glm::quat>& Values, float Time)
    {
        if (Times.empty() || Values.empty())
        {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        
        if (Times.size() == 1)
        {
            return Values[0];
        }
        
        if (Time <= Times[0])
        {
            return Values[0];
        }
        
        if (Time >= Times[Times.size() - 1])
        {
            return Values[Values.size() - 1];
        }
        
        for (size_t i = 0; i < Times.size() - 1; ++i)
        {
            if (Time >= Times[i] && Time < Times[i + 1])
            {
                float DeltaTime = Times[i + 1] - Times[i];
                float BlendFactor = (Time - Times[i]) / DeltaTime;
                
                return glm::slerp(Values[i], Values[i + 1], BlendFactor);
            }
        }
        
        return Values[Values.size() - 1];
    }
}
