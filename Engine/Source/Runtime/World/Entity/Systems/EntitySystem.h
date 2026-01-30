#pragma once

#include "SystemContext.h"
#include "Core/Engine/Engine.h"
#include "Scripting/ScriptTypes.h"
#include "World/Entity/Traits.h"


namespace Lumina
{
    
#define ENTITY_SYSTEM( ... )\
FUpdatePriorityList PriorityList = FUpdatePriorityList(__VA_ARGS__);

    namespace Meta
    {
        template<typename TSystem>
        concept HasStartup = requires(TSystem Sys, const FSystemContext& Context)
        {
            { Sys.Startup(Context) } noexcept -> std::same_as<void>;
        };
    
        template<typename TSystem>
        concept HasUpdate = requires(TSystem Sys, const FSystemContext& Context)
        {
            { Sys.Update(Context) } noexcept -> std::same_as<void>;
        };
    
        template<typename TSystem>
        concept HasTeardown = requires(TSystem Sys, const FSystemContext& Context)
        {
            { Sys.Teardown(Context) } noexcept -> std::same_as<void>;
        };
    
        template<typename TSystem>
        concept IsSystem = HasStartup<TSystem> || HasUpdate<TSystem> || HasTeardown<TSystem>;
    
        template<IsSystem TSystem>
        void RegisterECSSystem()
        {
            using namespace entt::literals;
            auto Factory = entt::meta_factory<TSystem>(GEngine->GetEngineMetaContext())
                .type()
                .template traits<ECS::ETraits::System>();
        
            Factory. template data<&TSystem::PriorityList>("PriorityList"_hs);
        
            if constexpr (HasStartup<TSystem>)
            {
                Factory. template func<&TSystem::Startup>("Startup"_hs);
            }
        
            if constexpr (HasUpdate<TSystem>)
            {
                Factory. template func<&TSystem::Update>("Update"_hs);
            }
        
            if constexpr (HasTeardown<TSystem>)
            {
                Factory. template func<&TSystem::Teardown>("Teardown"_hs);
            }
        }
    }
    
    struct FEntitySystemWrapper
    {
        friend class CWorld;
        
        const FUpdatePriorityList& GetUpdatePriorityList() const;
        void Startup(FSystemContext& SystemContext) noexcept;
        void Update(FSystemContext& SystemContext) noexcept;
        void Teardown(FSystemContext& SystemContext) noexcept;
        
    private:
        entt::meta_type Underlying;
    };
    
    struct FEntityScriptSystem
    {
        friend class CWorld;
        
        const FUpdatePriorityList& GetUpdatePriorityList() const;
        void Startup(FSystemContext& SystemContext) noexcept;
        void Update(FSystemContext& SystemContext) noexcept;
        void Teardown(FSystemContext& SystemContext) noexcept;
        
    private:
        Scripting::FLuaSystemScriptEntry ScriptSystem;
    };
}
