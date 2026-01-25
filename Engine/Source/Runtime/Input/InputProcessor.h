#pragma once
#include "Input/Input.h"
#include "Events/EventProcessor.h"

namespace Lumina
{
    class FInputProcessor : public IEventHandler
    {
    public:

        RUNTIME_API static FInputProcessor& Get();
        
		// IEventHandler interface
        bool OnEvent(FEvent& Event) override;
		// End of IEventHandler interface
    	
		RUNTIME_API FORCEINLINE double GetMouseX() const { return MouseX; }
		RUNTIME_API FORCEINLINE double GetMouseY() const { return MouseY; }
		RUNTIME_API FORCEINLINE double GetMouseDeltaX() const { return MouseDeltaX; }
        RUNTIME_API FORCEINLINE double GetMouseDeltaY() const { return MouseDeltaY; }
        
        RUNTIME_API FORCEINLINE Input::EKeyState GetKeyState(EKeyCode KeyCode) const { return KeyStates[static_cast<uint32>(KeyCode)]; }
		RUNTIME_API FORCEINLINE Input::EMouseState GetMouseButtonState(EMouseCode MouseCode) const { return MouseStates[static_cast<uint32>(MouseCode)]; }

        RUNTIME_API FORCEINLINE bool IsKeyDown(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down || GetKeyState(KeyCode) == Input::EKeyState::Repeated; }
		RUNTIME_API FORCEINLINE bool IsKeyUp(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Up; }
    	RUNTIME_API FORCEINLINE bool IsKeyPressed(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down; }
		RUNTIME_API FORCEINLINE bool IsKeyRepeated(EKeyCode KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Repeated; }

		RUNTIME_API FORCEINLINE bool IsMouseButtonDown(EMouseCode MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Down; }
		RUNTIME_API FORCEINLINE bool IsMouseButtonUp(EMouseCode MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Up; }

		RUNTIME_API void SetCursorMode(int Mode);

    	void EndFrame();

    private:

    	int DesiredInputMode = 0x00034001;

        double MouseX = 0.0;
        double MouseY = 0.0;
    	
        double MouseDeltaX = 0.0;
        double MouseDeltaY = 0.0;

        TArray<Input::EKeyState, (uint32)EKeyCode::Num>			KeyStates = {};
        TArray<Input::EMouseState, (uint32)EMouseCode::Num>		MouseStates = {};
    };
}
