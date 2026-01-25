#include "pch.h"
#include "SimpleAnimationSystem.h"

#include "Assets/AssetTypes/Mesh/Animation/Animation.h"
#include "assets/assettypes/mesh/skeletalmesh/skeletalmesh.h"
#include "Assets/AssetTypes/Mesh/Skeleton/Skeleton.h"
#include "Renderer/MeshData.h"
#include "world/Entity/Components/SimpleAnimationComponent.h"
#include "World/Entity/Components/SkeletalMeshComponent.h"

namespace Lumina
{
    void CSimpleAnimationSystem::Update(FSystemContext& SystemContext)
    {
        LUMINA_PROFILE_SCOPE();
        auto View = SystemContext.CreateView<SSimpleAnimationComponent, SSkeletalMeshComponent>();

        auto Handle = View.handle();
        if (Handle->empty())
        {
            return;
        }
        
        Task::ParallelFor(Handle->size(), [&](uint32 Index)
        {
            entt::entity Entity = (*Handle)[Index];

            if (!View.contains(Entity))
            {
                return;
            }
            
            SSimpleAnimationComponent& AnimationComponent = View.get<SSimpleAnimationComponent>(Entity);
            SSkeletalMeshComponent& SkeletalMeshComponent = View.get<SSkeletalMeshComponent>(Entity);
            
            if (!AnimationComponent.Animation.IsValid() || !SkeletalMeshComponent.SkeletalMesh.IsValid())
            {
                return;
            }
            
            CSkeletalMesh* Mesh = SkeletalMeshComponent.SkeletalMesh;
            if (!Mesh->Skeleton.IsValid())
            {
                return;
            }
            
            FSkeletonResource* Skeleton = Mesh->Skeleton->GetSkeletonResource();
            
            if (AnimationComponent.bPlaying)
            {
                AnimationComponent.CurrentTime += static_cast<float>(SystemContext.GetDeltaTime()) * AnimationComponent.PlaybackSpeed;
            }
            
            float AnimDuration = AnimationComponent.Animation->GetDuration();

            if (AnimationComponent.CurrentTime >= AnimDuration)
            {
                if (AnimationComponent.bLooping)
                {
                    AnimationComponent.CurrentTime = fmod(AnimationComponent.CurrentTime, AnimDuration);
                }
                else
                {
                    AnimationComponent.CurrentTime = AnimDuration;
                    AnimationComponent.bPlaying = false;
                }
            }
            
            AnimationComponent.Animation->SamplePose(AnimationComponent.CurrentTime, Skeleton, SkeletalMeshComponent.BoneTransforms);
        });
    }
}
