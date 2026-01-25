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

    EDITOR_API extern class FEditorEngine* GEditorEngine;
    
    class EDITOR_API FEditorEngine : public FEngine
    {
    public:
        bool Init() override;
        bool Shutdown() override;
        
        #if WITH_EDITOR
        IDevelopmentToolUI* CreateDevelopmentTools() override;
        #endif
        
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
    

    class EDITOR_API FLuminaEditor : public FApplication
    {
    public:
    
        FLuminaEditor();

        bool Initialize(int argc, char** argv) override;
        FEngine* CreateEngine() override;
        
        bool ShouldExit() const override;        
        void CreateProject();
        void OpenProject();
        FWindowSpecs GetWindowSpecs() const override;

        void RenderDeveloperTools(const FUpdateContext& UpdateContext) override;
        
        void Shutdown() override;
    
    private:
        
        
    };

    
    
}
