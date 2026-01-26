#pragma once
#include "Core/Module/ModuleInterface.h"

namespace Lumina
{
    class SANDBOX_API FSandboxModule : public IModuleInterface
    {
    public:
        
        void StartupModule() override;
        void ShutdownModule() override;
        
    };
}
