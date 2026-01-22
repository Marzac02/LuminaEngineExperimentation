#include "pch.h"
#include "TaskTypes.h"

#include "TaskSystem.h"
#include "Memory/Memory.h"

namespace Lumina
{
    void FCompletionActionDelete::OnDependenciesComplete(enki::TaskScheduler* pTaskScheduler_, uint32_t threadNum_)
    {
        ICompletable::OnDependenciesComplete(pTaskScheduler_, threadNum_);

        auto LambdaTask = static_cast<const FLambdaTask*>(Dependency.GetDependencyTask());
        if (auto Handle = LambdaTask->TaskHandle.lock())
        {
            Handle->bCompleted.exchange(true, std::memory_order_release);
            std::atomic_notify_all(&Handle->bCompleted);
        }
        
        Memory::Delete(Dependency.GetDependencyTask());
    }
}
