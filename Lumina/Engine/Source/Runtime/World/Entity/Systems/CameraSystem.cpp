#include "pch.h"
#include "CameraSystem.h"

#include "Input/InputProcessor.h"
#include "Renderer/RendererUtils.h"
#include "World/Entity/Components/CameraComponent.h"

namespace Lumina
{
    
    void CCameraSystem::RegisterEventListeners(FSystemContext& SystemContext)
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
            auto View = SystemContext.CreateView<SCameraControllerComponent>();

            View.each([&](SCameraControllerComponent& Controller)
            {
               glm::vec2 MouseDelta = glm::vec2(FInputProcessor::Get().GetMouseDeltaX(), FInputProcessor::Get().GetMouseDeltaY());

                Controller.Yaw -= MouseDelta.x * Controller.MouseSensitivity;

                float PitchDelta = MouseDelta.y * Controller.MouseSensitivity;

                if (Controller.bInvertY)
                {
                    PitchDelta = -PitchDelta;
                }

                Controller.Pitch = glm::clamp(Controller.Pitch + PitchDelta, Controller.MinPitch, Controller.MaxPitch);
            });
        }

        {
            auto View = SystemContext.CreateView<SCameraComponent, SCameraControllerComponent, SFirstPersonCameraComponent>();

            View.each([&](entt::entity Entity, SCameraComponent& Camera, const SCameraControllerComponent& Controller, const SFirstPersonCameraComponent& FPCamera)
            {
                glm::vec3 TargetPos(0.0f);
                if (Controller.TargetEntity != entt::null)
                {
                    if (STransformComponent* Transform = SystemContext.TryGet<STransformComponent>(Controller.TargetEntity))
                    {
                        TargetPos = Transform->GetLocation() + glm::vec3(0, FPCamera.EyeHeight, 0);
                    }
                }
                else
                {
                    if (STransformComponent* Transform = SystemContext.TryGet<STransformComponent>(Entity))
                    {
                        TargetPos = Transform->GetLocation() + glm::vec3(0, FPCamera.EyeHeight, 0);
                    }
                }

                glm::vec3 Forward = RenderUtils::GetForwardVector(Controller.Yaw, Controller.Pitch);
                glm::vec3 Up = glm::vec3(0, 1, 0);

                Camera.SetView(TargetPos, Forward, Up);
            });
            
        }
        
        {
            auto View = SystemContext.CreateView<SCameraComponent, STransformComponent>(entt::exclude<SCameraControllerComponent>);
            View.each([](SCameraComponent& CameraComponent, const STransformComponent& TransformComponent)
            {
                glm::vec3 UpdatedForward = TransformComponent.WorldTransform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 UpdatedUp      = TransformComponent.WorldTransform.Rotation * glm::vec3(0.0f, 1.0f,  0.0f);
            
                CameraComponent.SetView(TransformComponent.WorldTransform.Location, UpdatedForward, UpdatedUp);
            });
        }
    }
}
