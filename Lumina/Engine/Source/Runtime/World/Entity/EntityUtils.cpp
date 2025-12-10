#include "pch.h"
#include "EntityUtils.h"

#include "Components/DirtyComponent.h"
#include "Components/EditorComponent.h"
#include "Components/RelationshipComponent.h"
#include "Components/SingletonEntityComponent.h"
#include "components/tagcomponent.h"
#include "Core/Object/Class.h"

namespace Lumina::ECS::Utils
{
    
    bool SerializeEntity(FArchive& Ar, FEntityRegistry& Registry, entt::entity& Entity)
    {
        using namespace entt::literals;
        
        if (Ar.IsWriting())
        {
            Ar << Entity;

            bool bHasRelationship = false;
            int64 SizeBeforeRelationshipFlag = Ar.Tell();
            Ar << bHasRelationship;

            if (FRelationshipComponent* RelationshipComponent = Registry.try_get<FRelationshipComponent>(Entity))
            {
                bHasRelationship = true;
                
                int64 SizeBefore = Ar.Tell();
                Ar.Seek(SizeBeforeRelationshipFlag);
                Ar << bHasRelationship;
                Ar.Seek(SizeBefore);
                
                Ar << RelationshipComponent->Parent;
                Ar << RelationshipComponent->NumChildren;
                
                for (SIZE_T i = 0; i < RelationshipComponent->NumChildren; ++i)
                {
                    Ar << RelationshipComponent->Children[i];
                }
            }

            int64 NumComponentsPos = Ar.Tell();
            SIZE_T NumComponents = 0;
            Ar << NumComponents;

            for (auto [ID, Set] : Registry.storage())
            {
                if (Set.contains(Entity))
                {
                    void* ComponentPointer = Set.value(Entity);
                    if (auto ReturnValue = entt::resolve(Set.type()).invoke("staticstruct"_hs, {}))
                    {
                        void** Type = ReturnValue.try_cast<void*>();
                        if (CStruct* StructType = *(CStruct**)Type)
                        {
                            FName Name = StructType->GetQualifiedName();
                            Ar << Name;
                            
                            int64 ComponentStart = Ar.Tell();

                            int64 ComponentSize = 0;
                            Ar << ComponentSize;

                            int64 StartOfComponentData = Ar.Tell();

                            StructType->SerializeTaggedProperties(Ar, ComponentPointer);

                            int64 EndOfComponentData = Ar.Tell();

                            ComponentSize = EndOfComponentData - StartOfComponentData;

                            Ar.Seek(ComponentStart);
                            Ar << ComponentSize;
                            Ar.Seek(EndOfComponentData);
                            
                            NumComponents++;
                        }
                    }
                }
            }

            int64 SizeBefore = Ar.Tell();
            Ar.Seek(NumComponentsPos);    
            Ar << NumComponents;
            Ar.Seek(SizeBefore);
            
        }
        else if (Ar.IsReading())
        {
            uint32 EntityID = (uint32)Entity;
            Ar << EntityID;
            Entity = (entt::entity)EntityID;

            Entity = Registry.create(Entity);

            bool bHasRelationship = false;
            Ar << bHasRelationship;

            if (bHasRelationship)
            {
                FRelationshipComponent& RelationshipComponent = Registry.emplace<FRelationshipComponent>(Entity);
                Ar << RelationshipComponent.Parent;
                Ar << RelationshipComponent.NumChildren;
                
                for (SIZE_T i = 0; i < RelationshipComponent.NumChildren; ++i)
                {
                    Ar << RelationshipComponent.Children[i];
                }
            }

            SIZE_T NumComponents = 0;
            Ar << NumComponents;

            for (SIZE_T i = 0; i < NumComponents; ++i)
            {
                FName TypeName;
                Ar << TypeName;

                int64 ComponentSize = 0;
                Ar << ComponentSize;

                int64 ComponentStart = Ar.Tell();

                if (CStruct* Struct = FindObject<CStruct>(nullptr, TypeName))
                {
                    using namespace entt::literals;

                    if (Struct == STagComponent::StaticStruct())
                    {
                        STagComponent NewTagComponent;
                        Struct->SerializeTaggedProperties(Ar, &NewTagComponent);

                        Registry.storage<STagComponent>(entt::hashed_string(NewTagComponent.Tag.c_str())).emplace(Entity, NewTagComponent);
                    }
                    else
                    {
                        entt::hashed_string HashString(Struct->GetName().c_str());
                        if (entt::meta_type Meta = entt::resolve(HashString))
                        {
                            void* RegistryPtr = &Registry;
                            Meta.invoke("addcomponentwithmemory"_hs, {}, std::ref(Ar), Entity, RegistryPtr);
                        }
                    }
                }

                int64 ComponentEnd = ComponentSize + ComponentStart;
                Ar.Seek(ComponentEnd);
            }
        }
        
        return !Ar.HasError();
    }

    bool SerializeRegistry(FArchive& Ar, FEntityRegistry& Registry)
    {
        using namespace entt::literals;
        
        if (Ar.IsWriting())
        {
            Registry.compact<>();
            auto View = Registry.view<entt::entity>(entt::exclude<FEditorComponent, FSingletonEntityTag>);

            int64 PreSerializePos = Ar.Tell();
    
            int32 NumEntitiesSerialized = 0;
            Ar << NumEntitiesSerialized;

            View.each([&](entt::entity Entity)
            {
                int64 PreEntityPos = Ar.Tell();
        
                int64 EntitySaveSize = 0;
                Ar << EntitySaveSize;

                bool bSuccess = SerializeEntity(Ar, Registry, Entity);
                if (!bSuccess)
                {
                    // Rewind to before this entity's data and continue with next entity
                    Ar.Seek(PreEntityPos);
                    return;
                }

                NumEntitiesSerialized++;

                int64 PostEntityPos = Ar.Tell();

                // Calculate actual size written (excluding the size field itself)
                EntitySaveSize = PostEntityPos - PreEntityPos - sizeof(int64);
        
                // Go back and write the correct size
                Ar.Seek(PreEntityPos);
                Ar << EntitySaveSize;
        
                // Return to end position to continue with next entity
                Ar.Seek(PostEntityPos);
            });
    
            int64 PostSerializePos = Ar.Tell();

            // Go back and write the actual number of successfully serialized entities
            Ar.Seek(PreSerializePos);
            Ar << NumEntitiesSerialized;

            // Return to end of all serialized data
            Ar.Seek(PostSerializePos);
        }
        else if (Ar.IsReading())
        {
            int32 NumEntitiesSerialized = 0;
            Ar << NumEntitiesSerialized;

            for (int32 i = 0; i < NumEntitiesSerialized; ++i)
            {
                int64 EntitySaveSize = 0;
                Ar << EntitySaveSize;
        
                int64 PreEntityPos = Ar.Tell();

                entt::entity NewEntity = entt::null;
                bool bSuccess = ECS::Utils::SerializeEntity(Ar, Registry, NewEntity);
                
                if (!bSuccess || NewEntity == entt::null)
                {
                    // Skip to the next entity using the saved size
                    LOG_ERROR("Failed to serialize entity: {}", (int)NewEntity);
                    Ar.Seek(PreEntityPos + EntitySaveSize);
                    continue;
                }

                Registry.emplace_or_replace<FNeedsTransformUpdate>(NewEntity);
        
                int64 PostEntityPos = Ar.Tell();
                int64 ActualBytesRead = PostEntityPos - PreEntityPos;
        
                if (ActualBytesRead != EntitySaveSize)
                {
                    // Data mismatch, seek to correct position to stay aligned
                    LOG_ERROR("Entity Serialization Mismatch For {}: Expected: {} - Read: {}", (int)NewEntity, EntitySaveSize, ActualBytesRead);
                    Ar.Seek(PreEntityPos + EntitySaveSize);
                }
            }
        }

        return !Ar.HasError();
    }


    bool EntityHasTag(FName Tag, FEntityRegistry& Registry, entt::entity Entity)
    {
        return Registry.storage<STagComponent>(entt::hashed_string(Tag.c_str())).contains(Entity);
    }
}
