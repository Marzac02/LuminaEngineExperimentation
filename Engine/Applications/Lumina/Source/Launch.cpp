#include "Core/Application/Application.h"
#include "Core/Application/ApplicationGlobalState.h"
#include "Core/Windows/Window.h"

#if WITH_EDITOR
#include "LuminaEditor.h"
#endif

#include <print>

#include "Config/Config.h"
#include "Core/CommandLine/CommandLine.h"

using namespace Lumina;


int GuardedMain(int ArgC, char** ArgV)
{
    int Result = 0;
    try
    {
        FApplicationGlobalState GlobalState;
        
        FCommandLine Parsed{ArgC, ArgV};
        CommandLine = &Parsed;
        
        FApplication Application = FApplication{};
        GApp = &Application;
        
        FConfig Config = FConfig{};
        GConfig = &Config;
        
        TOptional<FFixedString> Project = CommandLine->Get("Project");
        
#if WITH_EDITOR
        GEditorEngine = new FEditorEngine{};
        GEngine = GEditorEngine;
#else
        GEngine = new FEngine{};
#endif
        
        if (Project)
        {
            GEngine->LoadProject(Project.value());
        }
        
        Result = Application.Run(ArgC, ArgV);
        
        delete GEngine;
    }
    catch (std::exception& Error)
    {
        std::println("Fatal error: {}", Error.what());
        return -1;
    }
    catch (...)
    {
        std::println("Unknown fatal error");
        return -1;
    }
	
    return Result;
}
