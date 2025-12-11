#include "pch.h"

#include "AssetManager.h"
#include "TaskScheduler.h"
#include "TaskSystem/TaskSystem.h"

namespace Lumina
{
    FAssetManager::FAssetManager()
    {
        
    }


    FAssetManager& FAssetManager::Get()
    {
        static FAssetManager Instance;
        return Instance;
    }
    
    TSharedPtr<FAssetRequest> FAssetManager::LoadAsset(const FString& PackagePath, const FGuid& RequestedAsset)
    {
        bool bAlreadyInQueue = false;
        TSharedPtr<FAssetRequest> ActiveRequest = TryFindActiveRequest(PackagePath, RequestedAsset, bAlreadyInQueue);
        
        if (!bAlreadyInQueue)
        {
            SubmitAssetRequest(ActiveRequest);
        }

        return ActiveRequest;
    }

    void FAssetManager::NotifyAssetRequestCompleted(const TSharedPtr<FAssetRequest>& Request)
    {
        auto It = eastl::find(ActiveRequests.begin(), ActiveRequests.end(), Request);
        Assert(It != ActiveRequests.end())

        for (auto& Functor : Request->Listeners)
        {
            Functor(Request->PendingObject);
        }
        
        ActiveRequests.erase(It);
    }

    void FAssetManager::FlushAsyncLoading()
    {
        while (OutstandingTasks.load(eastl::memory_order_acquire) != 0)
        {
            Threading::ThreadYield();
        }
    }

    TSharedPtr<FAssetRequest> FAssetManager::TryFindActiveRequest(const FString& InAssetPath, const FGuid& GUID, bool& bAlreadyInQueue)
    {
        auto It = eastl::find_if(ActiveRequests.begin(), ActiveRequests.end(), [&](const TSharedPtr<FAssetRequest>& Request)
        {
            return Request->GetAssetPath() == InAssetPath && Request->RequestedGUID == GUID;
        });

        if (It != ActiveRequests.end())
        {
            bAlreadyInQueue = true;
            return *It;
        }

        bAlreadyInQueue = false;
        TSharedPtr<FAssetRequest> NewRequest = MakeSharedPtr<FAssetRequest>(InAssetPath, GUID);
        ActiveRequests.emplace(NewRequest);
        return NewRequest;
        
    }

    void FAssetManager::SubmitAssetRequest(const TSharedPtr<FAssetRequest>& Request)
    {
        OutstandingTasks.fetch_add(1, eastl::memory_order_relaxed);

        struct FAssetTask : ITaskSet
        {
            FAssetManager* Manager;
            TSharedPtr<FAssetRequest> Request;

            FAssetTask(FAssetManager* InManager, const TSharedPtr<FAssetRequest>& InRequest)
                : Manager(InManager), Request(InRequest)
            {
                m_SetSize = 1;
            }

            void ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum) override
            {
                if (Request->Process())
                {
                    Manager->NotifyAssetRequestCompleted(Request);
                }

                Manager->OutstandingTasks.fetch_sub(1, eastl::memory_order_release);
            }
        };

        auto* Task = Memory::New<FAssetTask>(this, Request);
        GTaskSystem->ScheduleTask(Task);
    }
    
}

