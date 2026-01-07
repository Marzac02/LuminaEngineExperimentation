#include "pch.h"
#include "TaskTypes.h"

#include "TaskSystem.h"
#include "Memory/Memory.h"

namespace Lumina
{
    void FTaskCompletion::Wait() const
    {
        LUMINA_PROFILE_SCOPE();
        while (!bCompleted.load(eastl::memory_order_acquire))
        {
            Threading::ThreadYield();
        }
    }
    void FCompletionActionDelete::OnDependenciesComplete(enki::TaskScheduler* pTaskScheduler_, uint32_t threadNum_)
    {
        ICompletable::OnDependenciesComplete(pTaskScheduler_, threadNum_);

        auto LambdaTask = static_cast<const FLambdaTask*>(Dependency.GetDependencyTask());
        if (auto Handle = LambdaTask->TaskHandle.lock())
        {
            Handle->bCompleted.exchange(true, eastl::memory_order_release);
        }
        
        Memory::Delete(Dependency.GetDependencyTask());
    }
}
