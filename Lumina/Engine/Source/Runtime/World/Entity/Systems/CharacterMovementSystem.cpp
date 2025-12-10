#include "pch.h"
#include "CharacterMovementSystem.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Input/InputProcessor.h"
#include "Physics/API/Jolt/JoltUtils.h"
#include "world/entity/components/charactercomponent.h"
#include "World/Entity/Components/CharacterControllerComponent.h"
#include "world/entity/components/velocitycomponent.h"

namespace Lumina
{
    void CCharacterMovementSystem::Update(FSystemContext& SystemContext)
    {
        auto View = SystemContext.CreateView<SCharacterControllerComponent>();
        View.each([&](SCharacterControllerComponent& Controller)
        {
            glm::vec2 Move(0.0f);
            
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
                Move.x += 1.0f;
            }
            
            if (FInputProcessor::Get().GetKeyState(EKeyCode::D) != Input::EKeyState::Up)
            {
                Move.x -= 1.0f;
            }

            if (glm::length2(Move) > LE_SMALL_NUMBER)
            {
                Move = glm::normalize(Move);
            }
            
            Controller.MoveInput = Move;
            Controller.bJumpPressed = FInputProcessor::Get().GetKeyState(EKeyCode::Space) == Input::EKeyState::Down;
            
        });
    }
}
