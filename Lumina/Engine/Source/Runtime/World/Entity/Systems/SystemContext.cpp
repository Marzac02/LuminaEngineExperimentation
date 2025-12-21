#include "pch.h"
#include "SystemContext.h"
#include "sol/sol.hpp"
#include "World/World.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/LuaComponent.h"
#include "world/entity/components/namecomponent.h"

namespace Lumina
{
    FSystemContext::FSystemContext(CWorld* InWorld)
        : World(InWorld)
        , Registry(InWorld->EntityRegistry)
        , Dispatcher(InWorld->SingletonDispatcher)
    {
        
    }

    void FSystemContext::RegisterWithLua(sol::state& Lua)
    {
        Lua.new_usertype<FSystemContext>("SystemContext",
            sol::no_constructor,
            "GetDeltaTime",         &FSystemContext::GetDeltaTime,
            "GetTime",              &FSystemContext::GetTime,
            "GetUpdateStage",       &FSystemContext::GetUpdateStage,
            "GetTransform",         &FSystemContext::GetEntityTransform,
            "BindEvent",            &FSystemContext::BindLuaEvent,
            "Emplace",              &FSystemContext::LuaEmplace,
            "Get",                  &FSystemContext::LuaGet,
            "SetActiveCamera",      &FSystemContext::LuaSetActiveCamera,
            "TranslateEntity",      &FSystemContext::TranslateEntity,
            "SetEntityLocation",    &FSystemContext::SetEntityLocation,
            "SetEntityRotation",    &FSystemContext::SetEntityRotation,
            "SetEntityScale",       &FSystemContext::SetEntityScale,
            
            "DirtyTransform",       sol::overload(
                [&](FSystemContext& Self, entt::entity Entity)
                {
                    Self.MarkEntityTransformDirty(Entity);
                },
                [&](FSystemContext& Self, entt::entity Entity, EMoveMode MoveMode)
                {
                    Self.MarkEntityTransformDirty(Entity, MoveMode);
                },
                [&](FSystemContext& Self, entt::entity Entity, EMoveMode MoveMode, bool bActivate)
                {
                    Self.MarkEntityTransformDirty(Entity, MoveMode, bActivate);
                }),
                
            "CastRay",              sol::overload(
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                },
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, uint32 LayerMask)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug, LayerMask);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                },
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, uint32 LayerMask, uint32 IgnoreBody)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug, LayerMask, IgnoreBody);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                }));
        
        Lua.new_enum("EMoveMode",
            "Teleport",       EMoveMode::Teleport,
            "MoveKinematic",  EMoveMode::MoveKinematic,
            "ActivateOnly",   EMoveMode::ActivateOnly);
        
    }
    
    entt::runtime_view FSystemContext::CreateRuntimeView(const TVector<FName>& Components)
    {
        LUMINA_PROFILE_SCOPE();

        entt::runtime_view RuntimeView;
        
        for (const FName& ComponentName : Components)
        {
            entt::hashed_string HashedString = entt::hashed_string(ComponentName.c_str());
            entt::meta_type Meta = entt::resolve(HashedString);
            if (!Meta)
            {
                continue;
            }

            if (entt::basic_sparse_set<>* Storage = Registry.storage(Meta.info().hash()))
            {
                RuntimeView.iterate(*Storage);
            }
        }

        return RuntimeView;
    }

    entt::runtime_view FSystemContext::CreateRuntimeView(const TVector<entt::id_type>& Components)
    {
        entt::runtime_view RuntimeView;
        
        for (entt::id_type Type : Components)
        {
            entt::meta_type Meta = entt::resolve(Type);
            if (!Meta)
            {
                continue;
            }

            if (entt::basic_sparse_set<>* Storage = Registry.storage(Meta.info().hash()))
            {
                RuntimeView.iterate(*Storage);
            }
        }

        return RuntimeView;
    }

    TOptional<FRayResult> FSystemContext::CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, uint32 LayerMask, int64 IgnoreBody) const
    {
        return World->CastRay(Start, End, bDrawDebug, LayerMask, IgnoreBody);
    }

    STransformComponent& FSystemContext::GetEntityTransform(entt::entity Entity)
    {
        return Get<STransformComponent>(Entity);
    }

    void FSystemContext::TranslateEntity(entt::entity Entity, const glm::vec3& Translation)
    {
        Registry.get<STransformComponent>(Entity).Translate(Translation);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::SetEntityLocation(entt::entity Entity, const glm::vec3& Location)
    {
        Registry.get<STransformComponent>(Entity).SetLocation(Location);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::SetEntityRotation(entt::entity Entity, const glm::quat& Rotation)
    {
        Registry.get<STransformComponent>(Entity).SetRotation(Rotation);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::SetEntityScale(entt::entity Entity, const glm::vec3& Scale)
    {
        Registry.get<STransformComponent>(Entity).SetScale(Scale);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::MarkEntityTransformDirty(entt::entity Entity, EMoveMode MoveMode, bool bActivate)
    {
        EmplaceOrReplace<FNeedsTransformUpdate>(Entity, FNeedsTransformUpdate{MoveMode, bActivate});  
    }

    entt::entity FSystemContext::Create(const FName& Name, const FTransform& Transform) const
    {
        entt::entity EntityID = Registry.create();
        Registry.emplace<STransformComponent>(EntityID, Transform);
        Registry.emplace<SNameComponent>(EntityID, Name);
        Registry.emplace_or_replace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }

    void FSystemContext::LuaSetActiveCamera(uint32 Entity)
    {
        Dispatcher.trigger<FSwitchActiveCameraEvent>(FSwitchActiveCameraEvent{(entt::entity)Entity});
    }

    sol::object FSystemContext::LuaEmplace(entt::entity Entity, const sol::object& Component)
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;

        entt::id_type TypeID = ECS::DeduceType(Component);
        const entt::meta_any& MaybeAny = ECS::InvokeMetaFunc(TypeID, "emplace_lua"_hs, 
            entt::forward_as_meta(Registry), Entity, entt::forward_as_meta(Component), sol::state_view(Component.lua_state()));

        return MaybeAny ? MaybeAny.cast<sol::reference>() : sol::nil;
    }

    sol::variadic_results FSystemContext::LuaGet(entt::entity Entity, const sol::variadic_args& Args)
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;
        
        sol::variadic_results Results;
        Results.reserve(Args.size());
        for (const sol::object Proxy : Args)
        {
            entt::id_type TypeID = ECS::DeduceType(Proxy);
            if (const entt::meta_any& MaybeAny = ECS::InvokeMetaFunc(TypeID, "get_lua"_hs, entt::forward_as_meta(Registry), Entity, sol::state_view(Args.lua_state())))
            {
                Results.emplace_back(MaybeAny.cast<sol::reference>());
            }
            else
            {
                Results.emplace_back(sol::nil);
            }
        }
        
        return Results;
    }

    void FSystemContext::BindLuaEvent(sol::table Table, sol::function Function)
    {
        
    }
}
