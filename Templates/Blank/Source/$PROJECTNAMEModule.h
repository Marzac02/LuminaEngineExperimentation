#pragma once
#include "Core/Module/ModuleInterface.h"

namespace Lumina
{
    class $PROJECTNAME_API F$PROJECTNAMEModule : public IModuleInterface
    {
    public:
        
        void StartupModule() override;
        void ShutdownModule() override;
        
    };
}
