#include "pch.h"
#include "Application.h"
#include "Assets/AssetManager/AssetManager.h"
#include "Core/Module/ModuleManager.h"
#include "Core/Utils/CommandLineParser.h"
#include "Core/Windows/Window.h"
#include "Core/Windows/WindowTypes.h"
#include "FileSystem/FileSystem.h"
#include "Input/InputProcessor.h"
#include "Paths/Paths.h"

namespace Lumina
{
    LUMINA_API FApplication* GApp;
    FCommandLineParser FApplication::CommandLine;

    FApplication::FApplication(const FString& InApplicationName, uint32 AppFlags)
    {
        ApplicationName = InApplicationName;
        ApplicationFlags = AppFlags;
        GApp = this;
    }

    int32 FApplication::Run(int argc, char** argv)
    {
        LUMINA_PROFILE_SCOPE();
        CommandLine.Parse(argc, argv);
        
        LOG_TRACE("Initializing Application: {0}", ApplicationName);
        
        //---------------------------------------------------------------
        // Application initialization.
        //--------------------------------------------------------------

        PreInitStartup();
        CreateApplicationWindow();
        CreateEngine();

        EventProcessor.RegisterEventHandler(&FInputProcessor::Get());

        if (!Initialize(argc, argv))
        {
            Shutdown();
            return 1;
        }
        
        //---------------------------------------------------------------
        // Core application loop.
        //--------------------------------------------------------------

        bool bEngineWantsExit = false;
        while(!bEngineWantsExit)
        {
            LUMINA_PROFILE_FRAME();

            MainWindow->ProcessMessages();

            bool bApplicationWantsExit = ShouldExit();
            
            bEngineWantsExit = !GEngine->Update(bApplicationWantsExit);

            FInputProcessor::Get().EndFrame();
        }

        
        //---------------------------------------------------------------
        // Application shutdown.
        //--------------------------------------------------------------

        LOG_TRACE("Shutting down application: {0}", ApplicationName.c_str());
        
        Shutdown();
        
        GEngine->Shutdown();
        Memory::Delete(GEngine);
        
        MainWindow->Shutdown();
        Memory::Delete(MainWindow);
        
        return 0;
    }

    bool FApplication::HasAnyFlags(EApplicationFlags Flags)
    {
        return (ApplicationFlags & static_cast<uint32>(Flags)) != 0;
    }

    void FApplication::WindowResized(FWindow* Window, const glm::uvec2& Extent)
    {
        if (!Window->IsWindowMinimized())
        {
            GEngine->SetEngineViewportSize(Extent);

            OnWindowResized(Window, Extent);
        }
    }

    void FApplication::RequestExit()
    {
        GApp->bExitRequested = true;
    }
    
    void FApplication::PreInitStartup()
    {
        Paths::InitializePaths();
    }

    bool FApplication::CreateApplicationWindow()
    {
        FWindowSpecs AppWindowSpecs;
        AppWindowSpecs.Title = ApplicationName.c_str();

        (void)FWindow::OnWindowResized.AddMember(this, &FApplication::WindowResized);
        
        MainWindow = FWindow::Create(AppWindowSpecs);
        MainWindow->Init();
        
        Windowing::SetPrimaryWindowHandle(MainWindow);
        
        return true;
    }

    
    bool FApplication::FatalError(const FString& Error)
    {
        return false;
    }
    
}
