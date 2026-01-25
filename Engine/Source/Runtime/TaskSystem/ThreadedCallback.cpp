#include "pch.h"
#include "ThreadedCallback.h"

#include "Core/Templates/LuminaTemplate.h"

namespace Lumina::MainThread
{
    static TFixedVector<TMoveOnlyFunction<void()>, 8> Callbacks;
    static FMutex Mutex;
    
    void ProcessQueue()
    {
        FScopeLock Lock(Mutex);

        for (auto& Callback : Callbacks)
        {
            Callback();
        }
        
        Callbacks.clear();
    }

    void Enqueue(TMoveOnlyFunction<void()>&& Callback)
    {
        FScopeLock Lock(Mutex);

        Callbacks.emplace_back(Move(Callback));
        
    }
}
