#pragma once

#include "ModuleInterface.h"
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Memory/Memory.h"
#include "Memory/SmartPtr.h"


#define IMPLEMENT_MODULE(ModuleClass, ModuleName) \
    DECLARE_MODULE_ALLOCATOR_OVERRIDES() \
    extern "C" __declspec(dllexport) Lumina::IModuleInterface* InitializeModule() \
    { \
        Lumina::Memory::InitializeThreadHeap(); \
        return Lumina::Memory::New<ModuleClass>(); \
    } \
       extern "C" __declspec(dllexport) void ShutdownModule() \
    { \
        Lumina::Memory::ShutdownThreadHeap(); \
    } \


namespace Lumina
{
    struct RUNTIME_API FModuleInfo
    {
        FName ModuleName;
        TUniquePtr<IModuleInterface> ModuleInterface;
        void* ModuleHandle;
    };
    
    using ModuleInitFunc = IModuleInterface* (*)();
    using ModuleShutdownFunc = void (*)();
    
    class FModuleManager
    {
    public:
        
        static FModuleManager& Get();

        RUNTIME_API IModuleInterface* LoadModule(FStringView ModulePath);
        RUNTIME_API bool UnloadModule(FStringView ModuleName);

        void UnloadAllModules();


    private:

        FModuleInfo* GetOrCreateModuleInfo(const FName& ModuleName);


    private:

        THashMap<FName, FModuleInfo> ModuleHashMap;
        
    };
}
