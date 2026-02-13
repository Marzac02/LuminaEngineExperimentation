#pragma once
#include "Core/Module/ModuleInterface.h"

namespace Lumina
{
    class MyProject_API FMyProjectModule : public IModuleInterface
    {
    public:
        
        void StartupModule() override;
        void ShutdownModule() override;
        
    };
}
