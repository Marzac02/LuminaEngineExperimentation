#pragma once

#if LE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "Core/Application/ApplicationGlobalState.h"


extern Lumina::FApplication* Lumina::CreateApplication(int argc, char** argv);

inline int GuardedMain(int argc, char** argv)
{
#if LE_PLATFORM_WINDOWS
	SetUnhandledExceptionFilter(ExceptionHandler);
#endif
	
	int Result = 0;
	{
		Lumina::FApplicationGlobalState GlobalState;
		Lumina::FApplication* App = Lumina::CreateApplication(argc, argv);
		Result = App->Run(argc, argv);
	}
	
	return Result;
}

#if LE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return GuardedMain(__argc, __argv);
}
#endif

int main(int argc, char** argv)
{
	return GuardedMain(argc, argv);
}

