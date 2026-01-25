#pragma once

#include "Core/Module/ModuleInterface.h"

namespace Lumina
{
    class EDITOR_API FEditorModule : public IModuleInterface
    {
    public:

        void StartupModule() override;
        void ShutdownModule() override;
    
    };
}
