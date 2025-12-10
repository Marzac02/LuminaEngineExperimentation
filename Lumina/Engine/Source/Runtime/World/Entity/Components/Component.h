#pragma once

#include "EntityComponentRegistry.h"
#include "Core/Engine/Engine.h"
#include <sol/sol.hpp>
#include "Core/Serialization/Archiver.h"
#include "Scripting/Lua/Scripting.h"
#include "entt/entt.hpp"
#include "World/Entity/Registry/EntityRegistry.h"

namespace Lumina
{
#define ENTITY_COMPONENT(Type) \
    static void* GetComponentStructType() { return Type::StaticStruct(); } \
    static void* AddComponent(entt::entity e, void* Registry) { return &static_cast<Lumina::FEntityRegistry*>(Registry)->emplace_or_replace<Type>(e); } \
    static void* AddComponentWithMemory(FArchive& Ar, entt::entity e, void* Registry) \
    { \
        Type T; \
        StaticStruct()->SerializeTaggedProperties(Ar, &T); \
        return &static_cast<Lumina::FEntityRegistry*>(Registry)->emplace<Type>(e, T); \
    } \
    static sol::object ToLua(sol::state_view Lua, void* ComponentPtr) { return sol::make_object(Lua, std::ref(*static_cast<Type*>(ComponentPtr))); } \
    static void Serialize(FArchive& Ar, void* Data) \
    { \
        CStruct* Struct = StaticStruct(); \
        Struct->SerializeTaggedProperties(Ar, Data); \
    } \
    \
    \
    static void RegisterMeta() { \
        using namespace entt::literals; \
        entt::meta_factory<Type>(GEngine->GetEngineMetaContext()) \
        .type(#Type ## _hs) \
        .func<&Type::GetComponentStructType>("staticstruct"_hs) \
        .func<&Type::AddComponent>("addcomponent"_hs) \
    .func<&Type::AddComponent>("addcomponentwithmemory"_hs) \
        .func<&Type::ToLua>("tolua"_hs) \
        .func<&Type::Serialize>("serialize"_hs); \
    } \
    struct DeferredAutoRegister { \
        DeferredAutoRegister() { \
            Lumina::ECS::AddDeferredComponentRegistry(&Type::RegisterMeta); \
        } \
    }; \
    static inline DeferredAutoRegister DeferredAutoRegisterInstance;
}

template<typename T, typename = void>
struct THasSolComponentReflection : std::false_type {};

template<typename T>
struct THasSolComponentReflection<T, std::void_t<decltype(T::RegisterLua(std::declval<sol::state_view>()))>> : std::true_type {};

namespace Lumina::ECS
{
    LUMINA_API inline auto GetSharedMetaCtxHandle()
    {
        return entt::locator<entt::meta_ctx>::handle();
    }

    LUMINA_API inline void SetMetaContext(auto sharedCtx)
    {
        entt::locator<entt::meta_ctx>::reset(sharedCtx);
    }
}
