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
            "GetDeltaTime",     &FSystemContext::GetDeltaTime,
            "GetTime",          &FSystemContext::GetTime,
            "GetUpdateStage",   &FSystemContext::GetUpdateStage,
            "GetTransform",     &FSystemContext::GetEntityTransform,
            "BindEvent",        &FSystemContext::BindLuaEvent,
            "DirtyTransform",   &FSystemContext::MarkEntityTransformDirty,
            "Emplace",          &FSystemContext::LuaEmplace,
            "Get",              &FSystemContext::LuaGet,
            "SetActiveCamera",  &FSystemContext::LuaSetActiveCamera);
        
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

    STransformComponent& FSystemContext::GetEntityTransform(uint32 Entity)
    {
        return Get<STransformComponent>((entt::entity)Entity);
    }

    void FSystemContext::MarkEntityTransformDirty(uint32 Entity)
    {
        EmplaceOrReplace<FNeedsTransformUpdate>((entt::entity)(Entity));  
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
        const entt::meta_any& MaybeAny = ECS::InvokeMetaFunc(TypeID, "emplace_lua"_hs, entt::forward_as_meta(Registry), Entity, Component, sol::state_view(Component.lua_state()));

        return MaybeAny ? MaybeAny.cast<sol::reference>() : sol::nil;
    }

    sol::reference FSystemContext::LuaGet(entt::entity Entity, const sol::object& Type)
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;
        
        entt::id_type TypeID = ECS::DeduceType(Type);
        const entt::meta_any& MaybeAny = ECS::InvokeMetaFunc(TypeID, "get_lua"_hs, entt::forward_as_meta(Registry), Entity, sol::state_view(Type.lua_state()));

        return MaybeAny ? MaybeAny.cast<sol::reference>() : sol::nil;
    }

    void FSystemContext::BindLuaEvent(sol::table Table, sol::function Function)
    {
        
    }
}
