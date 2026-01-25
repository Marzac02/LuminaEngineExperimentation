#pragma once

namespace Lumina
{
    class RUNTIME_API IModuleInterface
    {
    public:

        IModuleInterface() = default;
        virtual ~IModuleInterface() = default;
        

        /** Called after the DLL has been loaded and module object has been created. */
        virtual void StartupModule() { }


        /** Called before the module is unloaded, right before the module object is destroyed */
        virtual void ShutdownModule() { }
        
    
    };
}