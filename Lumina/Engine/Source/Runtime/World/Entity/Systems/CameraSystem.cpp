#include "pch.h"
#include "CameraSystem.h"

#include "Input/InputProcessor.h"
#include "Renderer/RendererUtils.h"
#include "World/Entity/Components/CameraComponent.h"

namespace Lumina
{
    
    void CCameraSystem::Init(FSystemContext& SystemContext)
    {
        SystemContext.GetRegistry().on_construct<SCameraComponent>().connect<&ThisClass::NewCameraConstructed>(this);
    }

    void CCameraSystem::NewCameraConstructed(entt::registry& Registry, entt::entity Entity)
    {
        SCameraComponent& Camera = Registry.get<SCameraComponent>(Entity);
        if (Camera.bAutoActivate)
        {
            Registry.ctx().get<entt::dispatcher&>().trigger<FSwitchActiveCameraEvent>(FSwitchActiveCameraEvent{Entity});
        }
    }

    void CCameraSystem::Update(FSystemContext& SystemContext)
    {
        {
            auto View = SystemContext.CreateView<SCameraComponent, STransformComponent>();
            View.each([](SCameraComponent& CameraComponent, const STransformComponent& TransformComponent)
            {
                CameraComponent.SetView(TransformComponent.WorldTransform.Location, TransformComponent.GetForward(), TransformComponent.GetUp());
            });
        }
    }
}
