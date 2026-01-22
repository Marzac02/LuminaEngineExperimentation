#pragma once

#include "Core/Application/Application.h"
#include "Core/Delegates/Delegate.h"

DECLARE_MULTICAST_DELEGATE(FProjectLoadedDelegate);

namespace Lumina
{
    class FCamera;
    class FEditorSettings;
    class FEditorLayer;
    class FEditorPanel;

    extern class FEditorEngine* GEditorEngine;
    
    class FEditorEngine : public FEngine
    {
    public:
        bool Init() override;
        bool Shutdown() override;
        IDevelopmentToolUI* CreateDevelopmentTools() override;
    
        void LoadProject(FStringView Path);
        
        FORCEINLINE NODISCARD FStringView GetProjectName() const { return ProjectName; }
        FORCEINLINE NODISCARD FStringView GetProjectPath() const { return ProjectPath; }
        FORCEINLINE FProjectLoadedDelegate& GetProjectLoadedDelegate() { return OnProjectLoaded; }
        FORCEINLINE NODISCARD bool HasLoadedProject() const { return bHasLoadedProject; }
        NODISCARD FFixedString GetProjectScriptDirectory() const;
        NODISCARD FFixedString GetProjectGameDirectory() const;
        NODISCARD FFixedString GetProjectContentDirectory() const;
        
    private:
        
        void LoadProjectDLL();
        void LoadProjectScripts();
        
    private:
        
        FProjectLoadedDelegate OnProjectLoaded;
        
        FString ProjectName;
        FFixedString ProjectPath;
        
        bool bHasLoadedProject = false;
    };
    

    class LuminaEditor : public FApplication
    {
    public:
    
        LuminaEditor();

        bool Initialize(int argc, char** argv) override;
        FEngine* CreateEngine() override;
        
        bool ShouldExit() const override;        
        void CreateProject();
        void OpenProject();

        void RenderDeveloperTools(const FUpdateContext& UpdateContext) override;
        
        void Shutdown() override;
    
    private:
        
        
    };

    
    
}
