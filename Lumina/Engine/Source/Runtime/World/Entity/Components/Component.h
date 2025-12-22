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
    
    namespace Concept
    {
        template<typename T>
        concept TEventType = requires 
        { 
            { T::IsEvent } -> std::convertible_to<bool>; 
        } && T::IsEvent;
    }
    
    namespace Meta
    {
        inline bool IsValid(const entt::registry& Registry, entt::entity Entity)
        {
            return Registry.valid(Entity);
        }

        template<typename TComponent>
        bool HasComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.any_of<TComponent>(Entity);
        }

        template<typename TComponent>
        auto RemoveComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.remove<TComponent>(Entity);
        }

        template<typename TComponent>
        void ClearComponent(entt::registry& Registry)
        {
            Registry.clear<TComponent>();
        }

        template<typename TComponent>
        TComponent& EmplaceComponent(entt::registry& Registry, entt::entity Entity, const entt::meta_any& Any)
        {
            return Registry.emplace_or_replace<TComponent>(Entity, Any ? Any.cast<const TComponent&>() : TComponent{});
        }

        template<typename TComponent>
        TComponent& GetComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.get_or_emplace<TComponent>(Entity);
        }

        template<typename TComponent>
        void Serialize(FArchive& Ar, entt::meta_any& Any)
        {
            CStruct* Struct = TComponent::StaticStruct();
            TComponent& Instance = Any.cast<TComponent&>();
            Struct->SerializeTaggedProperties(Ar, &Instance);
        }

        template<typename TComponent>
        TComponent CreateInstance()
        {
            return TComponent{};
        }


        template<typename TComponent>
        CStruct* GetStructType()
        {
            return TComponent::StaticStruct();
        }

        // Begin Lua variants
    
        template<typename TComponent>
        sol::reference EmplaceComponentLua(entt::registry& Registry, entt::entity Entity, const sol::table& Instance, sol::state_view S)
        {
            auto& Component = Registry.emplace<TComponent>(Entity, Instance.valid() ? Move(Instance.as<TComponent&&>()) : TComponent{});
            return sol::make_reference(S, std::ref(Component));
        }
    
        template<typename TComponent>
        sol::reference GetComponentLua(entt::registry& Registry, entt::entity Entity, sol::state_view S)
        {
            auto& Component = Registry.get_or_emplace<TComponent>(Entity);
            return sol::make_reference(S, std::ref(Component));
        }
        
        template<typename TEvent>
        auto ConnectListener_Lua(entt::dispatcher& Dispatcher, const sol::function& Function)
        {
            struct FScriptListener
            {
                FScriptListener(entt::dispatcher& Dispatcher, const sol::function& Function)
                    : Callback(Function)
                {
                    Connection = Dispatcher.sink<TEvent>().template connect<&FScriptListener::Receive>(*this);
                }
                
                ~FScriptListener()
                {
                    Connection.release();
                    Callback.abandon();
                }
                
                LE_NO_COPYMOVE(FScriptListener);
                
                void Receive(const TEvent& Event)
                {
                    if (Connection && Callback.valid())
                    {
                        Callback(Event);
                    }
                }
                
                sol::function Callback;
                entt::connection Connection;
            };
            
            return std::make_unique<FScriptListener>(Dispatcher, Function);
        }

        template <typename TEvent>
        void TriggerEvent_Lua(entt::dispatcher& Dispatcher, const sol::table& Event) 
        {
            Dispatcher.trigger(Event.as<TEvent>());
        }
        
        template <typename TEvent>
        void EnqueueEvent_Lua(entt::dispatcher& Dispatcher, const sol::table& Event) 
        {
            Dispatcher.enqueue(Event.as<TEvent>());
        }
        
        // End lua variants

        template<typename TComponent>
        void RegisterComponentMeta()
        {
            using namespace entt::literals;
            entt::hashed_string TypeName = entt::hashed_string(TComponent::StaticStruct()->GetName().c_str());
            auto Meta = entt::meta_factory<TComponent>(GEngine->GetEngineMetaContext())
                .type(TypeName)
                .template func<&CreateInstance<TComponent>>("create_instance"_hs)
                .template func<&GetStructType<TComponent>>("static_struct"_hs)
                .template func<&HasComponent<TComponent>>("has"_hs)
                .template func<&GetComponent<TComponent>>("get"_hs)
                .template func<&RemoveComponent<TComponent>>("remove"_hs)
                .template func<&ClearComponent<TComponent>>("clear"_hs)
                .template func<&EmplaceComponent<TComponent>>("emplace"_hs)
                .template func<&Serialize<TComponent>>("serialize"_hs)
            
                .template func<&EmplaceComponentLua<TComponent>>("emplace_lua"_hs)
                .template func<&GetComponentLua<TComponent>>("get_lua"_hs)
            
                .template func<&ConnectListener_Lua<TComponent>>("connect_listener_lua"_hs)
                .template func<&TriggerEvent_Lua<TComponent>>("trigger_event_lua"_hs);
            
        }
        
        template<typename TComponent>
        struct TComponentAutoRegister
        {
            TComponentAutoRegister()
            {
                ECS::AddDeferredComponentRegistry(&RegisterComponentMeta<TComponent>); \
            }
        };
    }
}

    
#define ENTITY_COMPONENT(Type) \
static constexpr auto in_place_delete = true; \
static inline ::Lumina::Meta::TComponentAutoRegister<Type> DeferredAutoRegisterInstance{}; \
    
namespace Lumina::ECS
{
    NODISCARD inline entt::id_type GetTypeID(const sol::table& Table)
    {
        const auto F = Table["__type"].get<sol::function>();
        LUM_ASSERT(F.valid() && "__type not exposed to lua!")

        auto Name = F().get<const char*>();
        return entt::hashed_string(Name);
    }

    NODISCARD inline entt::id_type GetTypeID(FStringView Name)
    {
        return entt::hashed_string(Name.data());
    }

    template<typename T>
    NODISCARD entt::id_type DeduceType(T&& Obj)
    {
        switch (Obj.get_type())
        {
            case sol::type::number: return Obj.template as<entt::id_type>();
            case sol::type::table:  return GetTypeID(Obj);
            case sol::type::string: return GetTypeID(Obj.template as<const char*>());
        }

        LUMINA_NO_ENTRY()
    }

    NODISCARD inline auto CollectTypes(const sol::variadic_args& Args)
    {
        THashSet<entt::id_type> Types;
        
        eastl::transform(Args.cbegin(), Args.cend(), eastl::inserter(Types, Types.begin()), [](const sol::object& Object)
        {
            return DeduceType(Object);
        });
        
        return Types;
    }

    NODISCARD inline auto CollectTypes(const sol::table& Args)
    {
        THashSet<entt::id_type> Types;
        
        for (const auto& [key, value] : Args)
        {
            Types.insert(DeduceType(value));
        }
        
        return Types;
    }

    template<typename ... TArgs>
    entt::meta_any InvokeMetaFunc(const entt::meta_type& MetaType, entt::id_type FunctionID, TArgs&&... Args)
    {
        if (!MetaType)
        {
            return entt::meta_any{};
        }

        auto&& F = MetaType.func(FunctionID);
        if (!F)
        {
            return entt::meta_any{};
        }
        
        return F.invoke({}, Forward<TArgs>(Args)...);
    }

    template<typename ... TArgs>
    auto InvokeMetaFunc(const entt::id_type& TypeID, entt::id_type FunctionID, TArgs&&... Args)
    {
        return InvokeMetaFunc(entt::resolve(TypeID), FunctionID, Forward<TArgs>(Args)...);
    }
    
    LUMINA_API inline auto GetSharedMetaCtxHandle()
    {
        return entt::locator<entt::meta_ctx>::handle();
    }

    void SetMetaContext(auto SharedCtx)
    {
        entt::locator<entt::meta_ctx>::reset(SharedCtx);
    }
}
