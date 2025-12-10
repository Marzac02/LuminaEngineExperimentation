#pragma once
#include "Core/Serialization/Archiver.h"
#include "Registry/EntityRegistry.h"

namespace Lumina::ECS::Utils
{
    LUMINA_API bool SerializeEntity(FArchive& Ar, FEntityRegistry& Registry, entt::entity& Entity);
    LUMINA_API bool SerializeRegistry(FArchive& Ar, FEntityRegistry& Registry);
    LUMINA_API bool EntityHasTag(FName Tag, FEntityRegistry& Registry, entt::entity Entity);
    


    inline FArchive& operator << (FArchive& Ar, entt::entity& Entity)
    {
        uint32 UintEntity = (uint32)Entity;
        Ar << UintEntity;
        Entity = (entt::entity)UintEntity;
        
        return Ar;
    }
}
