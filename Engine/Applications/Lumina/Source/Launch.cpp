#include "LuminaEditor.h"
#include "Core/Application/Application.h"
#include "Core/Application/ApplicationGlobalState.h"

static int GuardedMain(int argc, char** argv)
{
    int Result = 0;
    try
    {
        Lumina::FApplicationGlobalState GlobalState;
        Lumina::FApplication* App = Lumina::Memory::New<Lumina::LuminaEditor>();
        Result = App->Run(argc, argv);
    }
    catch (...)
    {
        
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

DECLARE_MODULE_ALLOCATOR_OVERRIDES()
