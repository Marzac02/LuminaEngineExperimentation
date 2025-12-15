#include "pch.h"
#include "UpdateTransformEntitySystem.h"

#include <execution>
#include "glm/gtx/string_cast.hpp"
#include "TaskSystem/TaskSystem.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/Entity/Components/Transformcomponent.h"

namespace Lumina
{
    void CUpdateTransformEntitySystem::Update(FSystemContext& SystemContext)
    {
        LUMINA_PROFILE_SCOPE();

        auto SingleView = SystemContext.CreateView<FNeedsTransformUpdate, STransformComponent>(entt::exclude<FRelationshipComponent>);
        auto RelationshipGroup = SystemContext.CreateGroup<FNeedsTransformUpdate, FRelationshipComponent>(entt::get<STransformComponent>);
        
        if (!RelationshipGroup.empty())
        {
            TVector<entt::entity> DirtyEntities;
            for (auto entity : RelationshipGroup)
            {
                DirtyEntities.push_back(entity);
            }
            
            auto RelationshipTransformCallable = [&](uint32 Index)
            {
                entt::entity DirtyEntity = DirtyEntities[Index];
                
                // First, update the dirty entity itself
                auto& DirtyTransform = RelationshipGroup.get<STransformComponent>(DirtyEntity);
                auto& DirtyRelationship = RelationshipGroup.get<FRelationshipComponent>(DirtyEntity);
                
                if (DirtyRelationship.Parent != entt::null)
                {
                    glm::mat4 ParentWorldTransform         = SystemContext.Get<STransformComponent>(DirtyRelationship.Parent).WorldTransform.GetMatrix();
                    glm::mat4 LocalTransform               = DirtyTransform.Transform.GetMatrix();
                    DirtyTransform.WorldTransform           = FTransform(ParentWorldTransform * LocalTransform);
                }
                else
                {
                    DirtyTransform.WorldTransform = DirtyTransform.Transform;
                }
                
                DirtyTransform.CachedMatrix = DirtyTransform.WorldTransform.GetMatrix();
                
                // Recursively update only the children
                TFunction<void(entt::entity)> UpdateChildrenRecursive;
                UpdateChildrenRecursive = [&](entt::entity ParentEntity)
                {
                    auto& ParentRelationship = SystemContext.Get<FRelationshipComponent>(ParentEntity);
                    
                    for (uint8 i = 0; i < ParentRelationship.NumChildren; ++i)
                    {
                        entt::entity ChildEntity = ParentRelationship.Children[i];
                        
                        auto& ParentTransform = SystemContext.Get<STransformComponent>(ParentEntity);
                        auto& ChildTransform = SystemContext.Get<STransformComponent>(ChildEntity);

                        glm::mat4 ParentWorldTransform = ParentTransform.WorldTransform.GetMatrix();
                        glm::mat4 ChildLocalTransform = ChildTransform.Transform.GetMatrix();
                        
                        ChildTransform.WorldTransform = FTransform(ParentWorldTransform * ChildLocalTransform);
                        ChildTransform.CachedMatrix = ChildTransform.WorldTransform.GetMatrix();
                        
                        UpdateChildrenRecursive(ChildEntity);
                    }
                };
                
                // Update all children of the dirty entity
                UpdateChildrenRecursive(DirtyEntity);
            };
            
            // Only schedule tasks if there is a significant amount of transform updates required.
            if (DirtyEntities.size() > 1000)
            {
                Task::ParallelFor((uint32)DirtyEntities.size(), RelationshipTransformCallable);
            }
            else
            {
                for (uint32 i = 0; i < (uint32)DirtyEntities.size(); ++i)
                {
                    RelationshipTransformCallable(i);
                }
            }
        }

        if (SingleView.size_hint() < 1000)
        {
            SingleView.each([&](FNeedsTransformUpdate&, STransformComponent& TransformComponent)
            {
                TransformComponent.WorldTransform = TransformComponent.Transform;
                TransformComponent.CachedMatrix = TransformComponent.WorldTransform.GetMatrix();  
            });
        }
        else
        {
            auto WorkFunctor = [&](FNeedsTransformUpdate&, STransformComponent& Transform)
            {
                Transform.WorldTransform = Transform.Transform;
                Transform.CachedMatrix = Transform.Transform.GetMatrix();
            };

            auto Handle = SingleView.handle();
            Task::ParallelFor(Handle->size(), [&](uint32 Index)
            {
                entt::entity Entity = (*Handle)[Index];
                
                if (SingleView.contains(Entity))
                {
                    std::apply(WorkFunctor, SingleView.get(Entity));
                }
            });
        }
        
        SystemContext.Clear<FNeedsTransformUpdate>();
    }
}
