#include "pch.h"
#include "ModuleManager.h"
#include "ModuleInterface.h"
#include "Core/Delegates/CoreDelegates.h"
#include "Core/Templates/LuminaTemplate.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Platform/Platform.h"
#include "Platform/Process/PlatformProcess.h"


namespace Lumina
{
    FModuleManager& FModuleManager::Get()
    {
        static FModuleManager Instance;
        return Instance;
    }

    IModuleInterface* FModuleManager::LoadModule(FStringView ModulePath)
    {
        void* ModuleHandle = Platform::GetDLLHandle(StringUtils::ToWideString(ModulePath).c_str());

        if (!ModuleHandle)
        {
            LOG_WARN("Failed to load module: {}", ModulePath);
            return nullptr;
        }

        auto InitFunctionPtr = Platform::LumGetProcAddress<ModuleInitFunc>(ModuleHandle, "InitializeModule");
        if (!InitFunctionPtr)
        {
            LOG_WARN("Failed to get InitializeModule export: {}", ModulePath);
            return nullptr;
        }

        IModuleInterface* ModuleInterface = InitFunctionPtr();

        if (!ModuleInterface)
        {
            LOG_WARN("Module returned null from InitializeModule(): {}", ModulePath);
            return nullptr;
        }
        
        FStringView ModuleName = VFS::FileName(ModulePath, true);

        FModuleInfo* ModuleInfo = GetOrCreateModuleInfo(ModuleName);
        ModuleInfo->ModuleHandle = ModuleHandle;
        ModuleInfo->ModuleInterface.reset(ModuleInterface);

        ModuleInterface->StartupModule();
        
        LOG_INFO("[Module Manager] - Successfully loaded module {}", ModuleName);

        FCoreDelegates::OnModuleLoaded.Broadcast(ModuleInfo);
        
        return ModuleInterface;
    }

    bool FModuleManager::UnloadModule(FStringView ModuleName)
    {
        FName ModuleFName = FName(ModuleName);
        auto it = ModuleHashMap.find(ModuleFName);
        
        DEBUG_ASSERT(it != ModuleHashMap.end());

        FModuleInfo& Info = it->second;
        DEBUG_ASSERT(Info.ModuleInterface.get());
        
        Info.ModuleInterface->ShutdownModule();
        
        ModuleHashMap.erase(it);
        Info.ModuleInterface.reset();
        void* ModulePtr = Info.ModuleHandle;

        auto ShutdownFunctionPtr = Platform::LumGetProcAddress<ModuleShutdownFunc>(ModulePtr, "ShutdownModule");
        ShutdownFunctionPtr();
        
        Platform::FreeDLLHandle(ModulePtr);

        LOG_INFO("[Module Manager] - Successfully un-loaded module {}", ModuleName);
        
        return true;
    }

    void FModuleManager::UnloadAllModules()
    {
        TVector<FName> Keys;
        for (const auto& Pair : ModuleHashMap)
        {
            Keys.push_back(Pair.first);
        }

        //@TODO This causes a crash with the heap.
        for (const FName& Key : Keys)
        {
            UnloadModule(Key.ToString());
        }
    }

    FModuleInfo* FModuleManager::GetOrCreateModuleInfo(const FName& ModuleName)
    {
        auto it = ModuleHashMap.find(ModuleName);

        if (it != ModuleHashMap.end())
        {
            return &it->second;
        }

        FModuleInfo NewInfo;
        NewInfo.ModuleName = ModuleName;

        ModuleHashMap.emplace(ModuleName, Move(NewInfo));

        return &ModuleHashMap[ModuleName];
    }

}
