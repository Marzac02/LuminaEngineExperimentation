#pragma once
#include "Containers/String.h"
#include "Core/Delegates/Delegate.h"

namespace Lumina
{
    DECLARE_MULTICAST_DELEGATE(FOnProjectLoaded);
    
    class FProject
    {
    public:

        LUMINA_API FProject();
        
        struct FSettings
        {
            FString ProjectName;
            FString ProjectPath;
        };

        LUMINA_API void LoadProject(const FString& ProjectPath);
        
        LUMINA_API bool HasLoadedProject() const { return bHasProjectLoaded; }
        LUMINA_API const FString& GetProjectPath() const { return Settings.ProjectPath; }
        LUMINA_API FString GetProjectRootDirectory() const;
        
        LUMINA_API const FSettings& GetProjectSettings() const { return Settings; }

        LUMINA_API FString GetProjectContentDirectory() const;
        LUMINA_API FString GetProjectGameDirectory() const;
        LUMINA_API FString GetProjectScriptsDirectory() const;
        LUMINA_API FString GetProjectContentBinDirectory() const;

        FOnProjectLoaded OnProjectLoaded;

    private:
        
        FSettings           Settings;

        uint8               bHasProjectLoaded:1;
    };

}
