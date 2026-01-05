#pragma once

#include "Module/API.h"
#include <thread>
#include "Containers/Function.h"
#include "Containers/String.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/Thread.h"

namespace fs = std::filesystem;

namespace Lumina
{
    enum class LUMINA_API  EFileAction
    {
        Added,
        Modified,
        Removed,
        Renamed
    };
    
    struct LUMINA_API FFileEvent
    {
        FString Path;
        FString OldPath; // For renames
        EFileAction Action;
    };
    
    using FFileEventCallback = TFunction<void(const FFileEvent&)>;
    
    class LUMINA_API FDirectoryWatcher
    {
    public:
        
        FDirectoryWatcher();
        ~FDirectoryWatcher();

        bool Stop();
        bool Watch(const FFixedString& InPath, FFileEventCallback InCallback, bool bRecursive = true);
    
        bool IsRunning() const { return bRunning.load(Atomic::MemoryOrderRelaxed); }
    
    private:
        void WatchThreadFunc();
        FString WideToUtf8(const FWString& Wide);
        
        FFixedString Path;
        FFileEventCallback Callback;
        bool bWatchRecursive = true;
        TAtomic<bool> bRunning;
        FThread WatchThread;
    };
}