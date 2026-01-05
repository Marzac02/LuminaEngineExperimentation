#pragma once

#include "Core/Application/Application.h"
#include "Memory/SmartPtr.h"

namespace Lumina
{
    class FCamera;
    class FEditorSettings;
    class FEditorLayer;
    class FEditorPanel;

    inline class FEditorEngine* GEditorEngine = nullptr;
    
    class FEditorEngine : public FEngine
    {
    public:

        bool Init(FApplication* App) override;
        bool Shutdown() override;
        IDevelopmentToolUI* CreateDevelopmentTools() override;
    
        void LoadProject(FStringView Path);
        
        FORCEINLINE NODISCARD FStringView GetProjectName() const { return ProjectName; }
        FORCEINLINE NODISCARD FStringView GetProjectPath() const { return ProjectPath; }
        
    private:
    
        FString ProjectName;
        FFixedString ProjectPath;
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
