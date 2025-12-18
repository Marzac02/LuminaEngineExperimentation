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
    
    TSharedPtr<FAssetRequest> FAssetManager::LoadAssetAsync(const FString& PackagePath, const FGuid& RequestedAsset)
    {
        bool bAlreadyInQueue = false;
        TSharedPtr<FAssetRequest> ActiveRequest = CreateOrFindAssetRequest(PackagePath, RequestedAsset, bAlreadyInQueue);
        
        if (!bAlreadyInQueue)
        {
            SubmitAssetRequest(ActiveRequest);
        }

        return ActiveRequest;
    }

    CObject* FAssetManager::LoadAssetSynchronous(const FString& PackagePath, const FGuid& RequestedAsset)
    {
        bool bAlreadyInQueue = false;
        TSharedPtr<FAssetRequest> ActiveRequest = CreateOrFindAssetRequest(PackagePath, RequestedAsset, bAlreadyInQueue);
        
        if (bAlreadyInQueue)
        {
            FlushAsyncLoading();
            return ActiveRequest->GetPendingObject();
        }
        
        ActiveRequest->Process();
        NotifyAssetRequestCompleted(ActiveRequest);
        
        return ActiveRequest->GetPendingObject();
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
        GTaskSystem->WaitForAll();
    }

    TSharedPtr<FAssetRequest> FAssetManager::CreateOrFindAssetRequest(const FString& InAssetPath, const FGuid& GUID, bool& bAlreadyInQueue)
    {
        FScopeLock Lock(RequestMutex);
        
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
        struct FAssetTask : ITaskSet
        {
            FAssetManager*              Manager;
            TSharedPtr<FAssetRequest>   Request;
            FCompletionActionDelete     Deleter;


            FAssetTask(FAssetManager* InManager, const TSharedPtr<FAssetRequest>& InRequest)
                : ITaskSet(1)
                , Manager(InManager)
                , Request(InRequest)
            {
                Deleter.SetDependency(Deleter.Dependency, this);
            }

            void ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum) override
            {
                Request->Process();
                Manager->NotifyAssetRequestCompleted(Request);
            }
        };

        auto* Task = Memory::New<FAssetTask>(this, Request);

        GTaskSystem->ScheduleTask(Task);
    }
    
}

