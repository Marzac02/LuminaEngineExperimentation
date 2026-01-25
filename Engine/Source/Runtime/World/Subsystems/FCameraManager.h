#pragma once
#include "Core/Object/ObjectHandleTyped.h"
#include "World/Entity/Components/CameraComponent.h"


namespace Lumina
{
    class CWorld;

    class FCameraManager
    {
    public:

        FCameraManager(CWorld* InWorld)
            : WeakWorld(InWorld)
        {
            
        }
        
        FORCEINLINE void SetActiveCamera(entt::entity InEntity) { ActiveCameraEntity = InEntity; }
        
        FORCEINLINE entt::entity GetActiveCameraEntity() const { return ActiveCameraEntity; }
        SCameraComponent* GetCameraComponent() const;
    

    private:

        TWeakObjectPtr<CWorld> WeakWorld;
        entt::entity ActiveCameraEntity = entt::null;
    
    };
}
