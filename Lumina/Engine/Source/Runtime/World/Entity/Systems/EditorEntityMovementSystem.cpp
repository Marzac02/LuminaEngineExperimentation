#include "pch.h"
#include "EditorEntityMovementSystem.h"

#include <glm/gtx/string_cast.hpp>

#include "Input/InputProcessor.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Components/EditorComponent.h"
#include "World/Entity/Components/VelocityComponent.h"
#include <World/Entity/Components/DirtyComponent.h>

namespace Lumina
{

    void CEditorEntityMovementSystem::Update(FSystemContext& SystemContext)
    {
        LUMINA_PROFILE_SCOPE();
        
        double DeltaTime = SystemContext.GetDeltaTime();
        auto View = SystemContext.CreateView<STransformComponent, FEditorComponent, SCameraComponent, SVelocityComponent>();
        
        for (entt::entity EditorEntity : View)
        {
            STransformComponent& Transform      = View.get<STransformComponent>(EditorEntity);
            SVelocityComponent& Velocity        = View.get<SVelocityComponent>(EditorEntity);
            FEditorComponent& Editor            = View.get<FEditorComponent>(EditorEntity);

            if (!Editor.bEnabled)
            {
                return;
            }

            SystemContext.EmplaceOrReplace<FNeedsTransformUpdate>(EditorEntity);
            
            glm::vec3 Forward   = Transform.GetForward();
            glm::vec3 Right     = Transform.GetRight();
            glm::vec3 Up        = Transform.GetUp();
            
            float Speed = Velocity.Speed;
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::LeftShift))
            {
                Speed *= 10.0f;
            }
            
            glm::vec3 Acceleration(0.0f);
            
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::W))
            {
                Acceleration += Forward; // W = forward (+Z)
            }
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::S))
            {
                Acceleration -= Forward; // S = backward (-Z)
            }
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::D))
            {
                Acceleration -= Right; // D = right (+X)
            }
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::A))
            {
                Acceleration += Right; // A = left (-X)
            }
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::E))
            {
                Acceleration += Up; // E = up (+Y)
            }
            if (FInputProcessor::Get().IsKeyDown(EKeyCode::Q))
            {
                Acceleration -= Up; // Q = down (-Y)
            }
            
            if (glm::length(Acceleration) > 0.0f)
            {
                Acceleration = glm::normalize(Acceleration) * Speed;
            }
            
            Velocity.Velocity += Acceleration * static_cast<float>(DeltaTime);
            
            constexpr float Drag = 10.0f;
            Velocity.Velocity -= Velocity.Velocity * Drag * static_cast<float>(DeltaTime);
            
            Transform.Transform.Location += Velocity.Velocity * static_cast<float>(DeltaTime);
            
            if (FInputProcessor::Get().IsMouseButtonDown(EMouseCode::ButtonRight))
            {
                FInputProcessor::Get().SetCursorMode(GLFW_CURSOR_DISABLED);
            
                double MouseDeltaX = FInputProcessor::Get().GetMouseDeltaX();
				double MouseDeltaY = FInputProcessor::Get().GetMouseDeltaY();
                
                Transform.AddYaw(-MouseDeltaX * 0.1f);
                Transform.AddPitch(MouseDeltaY * 0.1f);
            }
            
            if (FInputProcessor::Get().IsMouseButtonUp(EMouseCode::ButtonRight))
            {
                FInputProcessor::Get().SetCursorMode(GLFW_CURSOR_NORMAL);
            }
        }
    }
}
