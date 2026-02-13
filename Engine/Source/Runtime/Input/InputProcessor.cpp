#include "pch.h"
#include "InputProcessor.h"
#include "Core/Windows/Window.h"
#include "Events/Event.h"

namespace Lumina
{
	FInputProcessor& FInputProcessor::Get()
	{
		static FInputProcessor Instance;
		return Instance;
	}

	bool FInputProcessor::OnEvent(FEvent& Event)
	{
		if (Event.IsA<FMouseMovedEvent>())
		{
			FMouseMovedEvent& MouseEvent = Event.As<FMouseMovedEvent>();
			MouseDeltaX += MouseEvent.GetDeltaX();
			MouseDeltaY += MouseEvent.GetDeltaY();

			MouseX = MouseEvent.GetX();
			MouseY = MouseEvent.GetY();
		}
		else if (Event.IsA<FMouseButtonPressedEvent>())
		{
			FMouseButtonEvent& MouseButtonEvent = Event.As<FMouseButtonPressedEvent>();
			uint32 MouseCode = static_cast<uint32>(MouseButtonEvent.GetButton());
			MouseStates[MouseCode] = (Input::EMouseState)GLFW_PRESS;
		}
		else if (Event.IsA<FMouseButtonReleasedEvent>())
		{
			FMouseButtonEvent& MouseButtonEvent = Event.As<FMouseButtonPressedEvent>();
			uint32 MouseCode = static_cast<uint32>(MouseButtonEvent.GetButton());
			MouseStates[MouseCode] = (Input::EMouseState)GLFW_RELEASE;
		}
		else if (Event.IsA<FKeyPressedEvent>())
		{
			FKeyPressedEvent& KeyEvent = Event.As<FKeyPressedEvent>();
			uint32 KeyCode = static_cast<uint32>(KeyEvent.GetKeyCode());
			KeyStates[KeyCode] = KeyEvent.IsRepeat() ? (Input::EKeyState)GLFW_REPEAT : (Input::EKeyState)GLFW_PRESS;
		}
		else if (Event.IsA<FKeyReleasedEvent>())
		{
			FKeyReleasedEvent& KeyEvent = Event.As<FKeyReleasedEvent>();
			uint32 KeyCode = static_cast<uint32>(KeyEvent.GetKeyCode());
			KeyStates[KeyCode] = (Input::EKeyState)GLFW_RELEASE;
		}
		else if (Event.IsA<FMouseScrolledEvent>())
		{
			FMouseScrolledEvent& MouseEvent = Event.As<FMouseScrolledEvent>();
			MouseZ = MouseEvent.GetOffset();
			
			uint32 KeyCode = static_cast<uint32>(MouseEvent.GetCode());

			//Gauge direction with offset.
			MouseStates[KeyCode] = MouseZ > 0.0 ? Input::EMouseState::Up : Input::EMouseState::Down;
		}

		// We never want to absorb here.
		return false;
	}

	void FInputProcessor::SetCursorMode(int Mode)
	{
		DesiredInputMode = Mode;
	}

	void FInputProcessor::EndFrame()
	{
		if (DesiredInputMode != glfwGetInputMode(Windowing::GetPrimaryWindowHandle()->GetWindow(), GLFW_CURSOR))
		{
			glfwSetInputMode(Windowing::GetPrimaryWindowHandle()->GetWindow(), GLFW_CURSOR, DesiredInputMode);
		}

		MouseDeltaX = 0.0;
		MouseDeltaY = 0.0;

		MouseZ = 0.0;
	}
}
