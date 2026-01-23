
#include "Sandbox.h"
#include "Core/Application/ApplicationGlobalState.h"
#include "Core/Windows/Window.h"
#include <Core/Windows/WindowTypes.h>
#if LE_PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include <cstdlib>
#include <Core/Application/Application.h>
#include <Core/Engine/Engine.h>
#include <Core/Object/ObjectBase.h>
#include <Core/UpdateContext.h>
#include <Memory/Memory.h>
#include <spdlog/details/windows_include.h>

using namespace Lumina;


FEngine* FSandbox::CreateEngine()
{
	GEngine = Memory::New<FSandboxEngine>();
	return GEngine;
}

void FSandboxEngine::OnUpdateStage(const FUpdateContext& Context)
{
	
}

bool FSandboxEngine::Init()
{
	InitializeCObjectSystem();

	return FEngine::Init();
}

bool FSandboxEngine::Shutdown()
{
	return true;
}

bool FSandbox::Initialize(int argc, char** argv)
{
	return GEngine->Init();
}

void FSandbox::Shutdown()
{
}

FWindowSpecs FSandbox::GetWindowSpecs() const
{
	FWindowSpecs AppWindowSpecs;
	AppWindowSpecs.Title = ApplicationName.c_str();
	AppWindowSpecs.bShowTitlebar = true;
	return AppWindowSpecs;
}

bool FSandbox::ShouldExit() const
{
	return MainWindow->ShouldClose() || bExitRequested;
}

static int GuardedMain(int argc, char** argv)
{
	int Result = 0;
	{
		FApplicationGlobalState GlobalState;
		FApplication* App = Memory::New<FSandbox>();
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


DECLARE_MODULE_ALLOCATOR_OVERRIDES()
