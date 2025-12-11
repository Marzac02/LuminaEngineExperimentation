#pragma once

#include "Containers/String.h"
#include "Core/Object/Object.h"
#include "TaskSystem/TaskSystem.h"

namespace Lumina
{
    class CObject;
    class CFactory;
    
    class FAssetRequest
    {
    public:

        friend class FAssetManager;

        
        FAssetRequest(const FString& InPath, const FGuid& GUID)
            : AssetPath(InPath)
            , RequestedGUID(GUID)
            , PendingObject(nullptr)
        {
        }

        FStringView GetAssetPath() const { return AssetPath; }
        CObject* GetPendingObject() const { return PendingObject; }
        void AddListener(const TFunction<void(CObject*)>& Functor) { Listeners.push_back(Functor); }
    
    private:

        bool Process();
        
    private:

        TVector<TFunction<void(CObject*)>>  Listeners;
        FString                             AssetPath;
        FGuid                               RequestedGUID;
        CObject*                            PendingObject;
    };
    
}
