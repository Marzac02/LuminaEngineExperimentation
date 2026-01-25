#include "Core/Application/Application.h"
#include "Core/Application/ApplicationGlobalState.h"
#include "Core/Windows/Window.h"

#if WITH_EDITOR
#include "LuminaEditor.h"
#endif

using namespace Lumina;


int GuardedMain(int ArgC, char** ArgV)
{
    int Result = 0;
    try
    {
        FApplicationGlobalState GlobalState;
        
        FApplication Application = FApplication{};
        GApp = &Application;
        
#if WITH_EDITOR
        GEditorEngine = new FEditorEngine{};
        GEngine = GEditorEngine;
#else
        GEngine = new FEngine{};
#endif
        
        Result = Application.Run(ArgC, ArgV);
        
        delete GEngine;
    }
    catch (std::exception& Error)
    {
        LOG_ERROR("Fatal error: {}", Error.what());
        return -1;
    }
    catch (...)
    {
        LOG_ERROR("Unknown fatal error");
        return -1;
    }
	
    return Result;
}
