#include "pch.h"
#include "EntitySystem.h"

#include "World/Entity/EntityUtils.h"

namespace Lumina
{
    using namespace entt::literals;

    const FUpdatePriorityList& FEntitySystemWrapper::GetUpdatePriorityList() const
    {
        return Underlying.data("PriorityList"_hs).get(Instance).cast<const FUpdatePriorityList&>();
    }

    void FEntitySystemWrapper::Startup(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Startup"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Update(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Update"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Teardown(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Teardown"_hs, entt::forward_as_meta(SystemContext));
    }

    uint64 FEntitySystemWrapper::GetHash() const noexcept
    {
        return Underlying.id();
    }

    uint64 FEntityScriptSystem::GetHash() const noexcept
    {
        return (uint64)&ScriptSystem;
    }

    const FUpdatePriorityList& FEntityScriptSystem::GetUpdatePriorityList() const
    {
        return ScriptSystem.PriorityList;
    }

    void FEntityScriptSystem::Startup(const FSystemContext& SystemContext) const noexcept
    {
        ScriptSystem.InitFunc(std::ref(SystemContext));
    }

    void FEntityScriptSystem::Update(const FSystemContext& SystemContext) const noexcept
    {
        LUMINA_PROFILE_SCOPE();
        LUMINA_PROFILE_TAG(std::format("Lua System: {}", ScriptSystem.Name).c_str());
        ScriptSystem.ExecuteFunc(std::ref(SystemContext));
    }

    void FEntityScriptSystem::Teardown(const FSystemContext& SystemContext) const noexcept
    {
        ScriptSystem.ShutdownFunc(std::ref(SystemContext));
    }
}
