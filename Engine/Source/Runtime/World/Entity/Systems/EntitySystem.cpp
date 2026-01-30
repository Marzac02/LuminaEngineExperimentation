#include "pch.h"
#include "EntitySystem.h"

#include "World/Entity/EntityUtils.h"

namespace Lumina
{
    using namespace entt::literals;

    const FUpdatePriorityList& FEntitySystemWrapper::GetUpdatePriorityList() const
    {
        auto Data = Underlying.data("PriorityList"_hs);
        return Data.get({}).cast<const FUpdatePriorityList&>();
    }

    void FEntitySystemWrapper::Startup(FSystemContext& SystemContext) noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Startup"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Update(FSystemContext& SystemContext) noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Update"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Teardown(FSystemContext& SystemContext) noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Teardown"_hs, entt::forward_as_meta(SystemContext));
    }

    const FUpdatePriorityList& FEntityScriptSystem::GetUpdatePriorityList() const
    {
        return ScriptSystem.PriorityList;
    }

    void FEntityScriptSystem::Startup(FSystemContext& SystemContext)
    {
        ScriptSystem.InitFunc(std::ref(SystemContext));
    }

    void FEntityScriptSystem::Update(FSystemContext& SystemContext)
    {
        ScriptSystem.ExecuteFunc(std::ref(SystemContext));
    }

    void FEntityScriptSystem::Teardown(FSystemContext& SystemContext)
    {
        ScriptSystem.ShutdownFunc(std::ref(SystemContext));
    }
}
