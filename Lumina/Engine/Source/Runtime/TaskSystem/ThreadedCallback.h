#pragma once
#include "Containers/Function.h"
#include "Module/API.h"

namespace Lumina::MainThread
{
    void ProcessQueue();

    /**
     * Enqueues a callback to be executed on the main thread at the start of the next frame.
     *
     * Thread-safe: may be called from any thread.
     * Execution order: callbacks are executed in FIFO order relative to other enqueued events.
     * Lifetime: the callback is moved into the queue and must not capture references
     *           that may be invalid by the time it executes.
     *
     * @param Callback Move-only callable invoked exactly once on the main thread.
     */
    LUMINA_API void Enqueue(TMoveOnlyFunction<void()>&& Callback);
    
}
