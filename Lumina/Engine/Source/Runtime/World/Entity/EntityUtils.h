#pragma once
#include "Core/Serialization/Archiver.h"
#include "Registry/EntityRegistry.h"

namespace Lumina::ECS::Utils
{
    LUMINA_API bool SerializeEntity(FArchive& Ar, FEntityRegistry& Registry, entt::entity& Entity);
    LUMINA_API bool SerializeRegistry(FArchive& Ar, FEntityRegistry& Registry);
    LUMINA_API bool EntityHasTag(FName Tag, FEntityRegistry& Registry, entt::entity Entity);
    
    template<typename TFunc>
    requires(eastl::is_invocable_v<TFunc, void*, entt::meta_type>)
    void ForEachComponent(TFunc&& Func, FEntityRegistry& Registry, entt::entity Entity)
    {
        for (auto&& [ID, Storage] : Registry.storage())
        {
            if (Storage.contains(Entity))
            {
                entt::meta_type MetaType = entt::resolve(Storage.info());
                eastl::invoke(Func, Storage.value(Entity), MetaType);
            }
        }
    }
    
    inline FArchive& operator << (FArchive& Ar, entt::entity& Entity)
    {
        uint32 UintEntity = static_cast<uint32>(Entity);
        Ar << UintEntity;
        Entity = static_cast<entt::entity>(UintEntity);
        
        return Ar;
    }
}
