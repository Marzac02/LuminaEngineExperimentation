#pragma once

#include "WindowTypes.h"
#include "Core/Delegates/Delegate.h"

#ifdef _WIN32
	#include <Windows.h>
#endif
#include "GLFW/glfw3.h"

namespace Lumina
{
	class FWindow;

	DECLARE_MULTICAST_DELEGATE(FWindowResizeDelegate, FWindow*, const glm::uvec2&);
	
	class FWindow
	{
	public:
		
		static FWindow* Create(const FWindowSpecs& InSpecs);
		
		FWindow(const FWindowSpecs& InSpecs)
			: LastMouseX(0)
			, LastMouseY(0)
			, Specs(InSpecs)
		{}

		virtual ~FWindow();
		LE_NO_COPYMOVE(FWindow);

		void Init();
		void Shutdown();
		void ProcessMessages();

		GLFWwindow* GetWindow() const { return Window; }

		LUMINA_API glm::uvec2 GetExtent() const;
		LUMINA_API uint32 GetWidth() const;
		LUMINA_API uint32 GetHeight() const;

		LUMINA_API void GetWindowPosition(int& X, int& Y);
		LUMINA_API void SetWindowPosition(int X, int Y);
		
		LUMINA_API void SetWindowSize(int X, int Y);

		LUMINA_API bool ShouldClose() const;
		LUMINA_API bool IsWindowMinimized() const;
		LUMINA_API bool IsWindowMaximized() const;
		LUMINA_API void Minimize();
		LUMINA_API void Restore();
		LUMINA_API void Maximize();
		LUMINA_API void Close();

		
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void MousePosCallback(GLFWwindow* window, double xpos, double ypos);
		static void KeyCallback(GLFWwindow* window, int Key, int Scancode, int Action, int Mods);
		
		static void WindowResizeCallback(GLFWwindow* window, int width, int height);
		static void WindowDropCallback(GLFWwindow* Window, int PathCount, const char* Paths[]);
		static void WindowCloseCallback(GLFWwindow* window);
		
		LUMINA_API static FWindowResizeDelegate OnWindowResized;

	private:

		bool bFirstMouseUpdate = true;
		double LastMouseX, LastMouseY;
		
		bool bInitialized = false;
		GLFWwindow* Window = nullptr;
		FWindowSpecs Specs;
	};

	namespace Windowing
	{
		LUMINA_API extern FWindow* PrimaryWindow;
		LUMINA_API FWindow* GetPrimaryWindowHandle();
		void SetPrimaryWindowHandle(FWindow* InWindow);
	}
	
}
