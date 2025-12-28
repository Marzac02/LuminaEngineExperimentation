#pragma once

#include "Component.h"
#include "Events/KeyCodes.h"
#include "Input/Input.h"
#include "Events/MouseCodes.h"
#include "Input/InputProcessor.h"
#include "InputComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SInputComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SInputComponent)
        
        FUNCTION(Script)
        FORCEINLINE double GetMouseX() const { return FInputProcessor::Get().GetMouseX(); }
        
        FUNCTION(Script)
        FORCEINLINE double GetMouseY() const { return FInputProcessor::Get().GetMouseY(); }
        
        FUNCTION(Script)
        FORCEINLINE double GetMouseDeltaX() const { return FInputProcessor::Get().GetMouseDeltaX(); }
        
        FUNCTION(Script)
        FORCEINLINE double GetMouseDeltaY() const { return FInputProcessor::Get().GetMouseDeltaY(); }
        
        FUNCTION(Script)
        FORCEINLINE Input::EKeyState GetKeyState(EKeyCode KeyCode) const { return FInputProcessor::Get().GetKeyState(KeyCode); }
        
        FUNCTION(Script)
        FORCEINLINE Input::EMouseState GetMouseButtonState(EMouseCode MouseCode) const { return FInputProcessor::Get().GetMouseButtonState(MouseCode); }

        FUNCTION(Script)
        FORCEINLINE bool IsKeyDown(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down || GetKeyState(KeyCode) == Input::EKeyState::Repeated; }
        
        FUNCTION(Script)
        FORCEINLINE bool IsKeyUp(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Up; }
        
        FUNCTION(Script)
        FORCEINLINE bool IsKeyPressed(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down; }
        
        FUNCTION(Script)
        FORCEINLINE bool IsKeyRepeated(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Repeated; }

        FUNCTION(Script)
        FORCEINLINE bool IsMouseButtonDown(EMouseCode MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Down; }
        
        FUNCTION(Script)
        FORCEINLINE bool IsMouseButtonUp(EMouseCode MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Up; }
        
        PROPERTY(Script)
        bool bInputEnabled = false;
    };
}
