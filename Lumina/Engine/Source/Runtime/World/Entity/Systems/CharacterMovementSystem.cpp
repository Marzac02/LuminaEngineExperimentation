#include "pch.h"
#include "CharacterMovementSystem.h"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Input/InputProcessor.h"
#include "World/Entity/Components/CameraComponent.h"
#include "world/entity/components/charactercomponent.h"
#include "World/Entity/Components/CharacterControllerComponent.h"

namespace Lumina
{
    void CPlayerCharacterMovementSystem::Update(FSystemContext& SystemContext)
    {
        auto View = SystemContext.CreateView<SCharacterPhysicsComponent, SCharacterMovementComponent, SCharacterControllerComponent>();
        View.each([&](entt::entity Entity, SCharacterPhysicsComponent& PhysicsComponent, SCharacterMovementComponent& MovementComponent, SCharacterControllerComponent& Controller)
        {
            glm::vec2 Move(0.0f);
    
            // Get input
            if (FInputProcessor::Get().GetKeyState(EKeyCode::W) != Input::EKeyState::Up)
            {
                Move.y += 1.0f;
            }
            if (FInputProcessor::Get().GetKeyState(EKeyCode::S) != Input::EKeyState::Up)
            {
                Move.y -= 1.0f;
            }
            if (FInputProcessor::Get().GetKeyState(EKeyCode::A) != Input::EKeyState::Up)
            {
                Move.x -= 1.0f;  // Left
            }
            if (FInputProcessor::Get().GetKeyState(EKeyCode::D) != Input::EKeyState::Up)
            {
                Move.x += 1.0f;  // Right
            }

            SCameraControllerComponent* CameraControllerComponent = SystemContext.TryGet<SCameraControllerComponent>(Entity);
            
            if (CameraControllerComponent)
            {
                float CameraYaw = CameraControllerComponent->Yaw;

                glm::vec3 Forward(glm::sin(glm::radians(CameraYaw)), 0.0f, glm::cos(glm::radians(CameraYaw)));
                glm::vec3 LookDirection = Forward;
                LookDirection = glm::normalize(LookDirection);
                
                float TargetYaw = glm::degrees(glm::atan(LookDirection.x, LookDirection.z));
                TargetYaw = glm::mod(TargetYaw, 360.0f);
                if (TargetYaw < 0.0f)
                {
                    TargetYaw += 360.0f;
                }
                
                JPH::Quat TargetRotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), glm::radians(TargetYaw));
                PhysicsComponent.Character->SetRotation(TargetRotation);
            }
            
            if (glm::length2(Move) > LE_SMALL_NUMBER)
            {
                Move = glm::normalize(Move);
                
                if (Controller.bUseControllerRotation)
                {
                    if (!CameraControllerComponent)
                    {
                        return;
                    }
                    
                    float CameraYaw = CameraControllerComponent->Yaw;
                    
                    glm::vec3 Forward(
                        glm::sin(glm::radians(CameraYaw)),
                        0.0f,
                        glm::cos(glm::radians(CameraYaw))
                    );
                    
                    glm::vec3 Right(
                        glm::cos(glm::radians(CameraYaw)),
                        0.0f,
                        -glm::sin(glm::radians(CameraYaw))
                    );
                    
                    glm::vec3 MoveDirection = Forward * Move.y + -Right * Move.x;
                    MoveDirection = glm::normalize(MoveDirection);
                    
                    Controller.MoveInput = glm::vec2(MoveDirection.x, MoveDirection.z);
                }
                else
                {
                    Controller.MoveInput = Move;
                }
            }
            else
            {
                Controller.MoveInput = glm::vec2(0.0f);
            }
            
            Controller.bJumpPressed = FInputProcessor::Get().GetKeyState(EKeyCode::Space) == Input::EKeyState::Down;
            MovementComponent.bWantsToJump = Controller.bJumpPressed;
        });
    }
}
