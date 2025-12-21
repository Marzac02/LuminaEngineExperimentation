#pragma once
#include "Physics/PhysicsTypes.h"
#include "Physics/Ray/RayCast.h"
#include "TaskSystem/TaskSystem.h"
#include "World/Entity/Components/TransformComponent.h"


namespace Lumina
{
    enum class EMoveMode;

    namespace Physics
    {
        class IPhysicsScene;
    }

    struct FSystemContext : INonCopyable
    {
        friend class CWorld;
        
        FSystemContext(CWorld* InWorld);
        ~FSystemContext() = default;
        
        static void RegisterWithLua(sol::state& Lua);

        LUMINA_API FORCEINLINE double GetDeltaTime() const { return DeltaTime; }
        LUMINA_API FORCEINLINE double GetTime() const { return Time; }
        LUMINA_API FORCEINLINE EUpdateStage GetUpdateStage() const { return UpdateStage; }


        template<typename T>
        NODISCARD auto EventSink(entt::id_type ID = entt::type_hash<T>())
        {
            return Dispatcher.sink<T>(ID);
        }
        
        template<typename T, typename ... TArgs>
        void EnqueueEvent(const entt::id_type& IDType = entt::type_hash<T>::value(), TArgs&&... Args)
        {
            Dispatcher.enqueue_hint<T, TArgs...>(IDType, Forward<TArgs>(Args)...);
        }

        template<typename T, typename ... TArgs>
        void DispatchEvent(const entt::id_type& IDType = entt::type_hash<T>::value(), TArgs&&... Args)
        {
            Dispatcher.trigger<T, TArgs...>(IDType, Forward<TArgs>(Args)...);
        }

        template<typename T, typename ... TArgs>
        void DispatchEvent(TArgs&&... Args)
        {
            Dispatcher.trigger<T, TArgs...>(Forward<TArgs>(Args)...);
        }
        
        template<typename... Ts, typename... TArgs>
        NODISCARD auto CreateView(TArgs&&... Args) -> decltype(std::declval<entt::registry>().view<Ts...>(std::forward<TArgs>(Args)...))
        {
            return Registry.view<Ts...>(std::forward<TArgs>(Args)...);
        }
        
        template<typename... Ts, typename TFunc, typename... TArgs>
        void ForEach(TFunc&& Function, TArgs&&... Args)
        {
            auto View = Registry.view<Ts...>(std::forward<TArgs>(Args)...);
            View.each(Forward<TFunc>(Function));
        }

        template<typename... Ts, typename TFunc, typename... TArgs>
        void ParallelForEach(TFunc&& Function, TArgs&&... Args)
        {
            auto View = Registry.view<Ts...>(std::forward<TArgs>(Args)...);
            auto Entities = View.handle();
    
            Task::ParallelFor(Entities.size(), Entities.size(), [&, View](uint32 Index)
            {
                entt::entity EntityID = (*Entities)[Index];
                
                if (View.contains(EntityID))
                {
                    std::apply(Function, View.get(EntityID));
                }
            });
        }

        NODISCARD auto GetRegistryContext() const
        {
            return Registry.ctx();
        }

        NODISCARD entt::registry& GetRegistry() const
        {
            return Registry;
        }
        
        template<typename... Ts, typename ... TArgs>
        NODISCARD auto CreateGroup(TArgs&&... Args)
        {
            return Registry.group<Ts...>(std::forward<TArgs>(Args)...);
        }

        template<typename... Ts>
        NODISCARD decltype(auto) Get(entt::entity entity) const
        {
            return Registry.get<Ts...>(entity);
        }

        template<typename... Ts>
        NODISCARD decltype(auto) TryGet(entt::entity entity) const
        {
            return Registry.try_get<Ts...>(entity);
        }
        
        template<typename... Ts>
        void Clear() const
        {
            Registry.clear<Ts...>();
        }

        template<typename... Ts>
        NODISCARD bool HasAnyOf(entt::entity EntityID) const
        {
            return Registry.any_of<Ts...>(EntityID);
        }

        template<typename ... Ts>
        NODISCARD bool HasAllOf(entt::entity EntityID) const
        {
            return Registry.any_of<Ts...>(EntityID);
        }

        template<typename T, typename ... TArgs>
        T& Emplace(entt::entity entity, TArgs&& ... Args)
        {
            return Registry.emplace<T>(entity, std::forward<TArgs>(Args)...);
        }

        template<typename T, typename ... TArgs>
        T& EmplaceOrReplace(entt::entity entity, TArgs&& ... Args)
        {
            return Registry.emplace_or_replace<T>(entity, std::forward<TArgs>(Args)...);
        }

        LUMINA_API TOptional<FRayResult> CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug = false, uint32 LayerMask = 0xFFFFFFFF, int64 IgnoreBody = -1) const;

        LUMINA_API STransformComponent& GetEntityTransform(entt::entity Entity) const;
        
        LUMINA_API glm::vec3 TranslateEntity(entt::entity Entity, const glm::vec3& Translation);
        LUMINA_API void SetEntityLocation(entt::entity Entity, const glm::vec3& Location);
        LUMINA_API void SetEntityRotation(entt::entity Entity, const glm::quat& Rotation);
        LUMINA_API void SetEntityScale(entt::entity Entity, const glm::vec3& Scale);
        
        LUMINA_API void MarkEntityTransformDirty(entt::entity Entity, EMoveMode MoveMode = EMoveMode::Teleport, bool bActivate = true);
        
        LUMINA_API entt::runtime_view CreateRuntimeView(const TVector<FName>& Components);
        LUMINA_API entt::runtime_view CreateRuntimeView(const TVector<entt::id_type>& Components);
        LUMINA_API entt::runtime_view CreateRuntimeView(const THashSet<entt::id_type>& Components);

        LUMINA_API entt::entity Create(const FName& Name, const FTransform& Transform = FTransform()) const;
        LUMINA_API entt::entity Create() const { return Registry.create(); }
        LUMINA_API void Destroy(entt::entity Entity) const { Registry.destroy(Entity); }

        LUMINA_API size_t GetNumEntities() const;
        LUMINA_API bool IsValidEntity(entt::entity Entity) const;
        
    private:
        
        bool Lua_HasAllOf(entt::entity Entity, const sol::variadic_args& Args);
        bool Lua_HasAnyOf(entt::entity Entity, const sol::variadic_args& Args);
        bool Lua_Has(entt::entity Entity, const sol::object& Type);
        
        void Lua_View(const sol::variadic_args& Args, const sol::function& Callback);
        void Lua_SetActiveCamera(uint32 Entity);
        sol::object Lua_Emplace(entt::entity Entity, const sol::object& Component);
        sol::variadic_results Lua_Get(entt::entity Entity, const sol::variadic_args& Args);
        void Lua_BindEvent(sol::table Table, sol::function Function);
        
    private:

        double                  DeltaTime = 0.0;
        double                  Time = 0.0;
        CWorld*                 World = nullptr;
        entt::registry&         Registry;
        entt::dispatcher&       Dispatcher;
        EUpdateStage            UpdateStage = EUpdateStage::FrameStart;
    };
    
    
}
