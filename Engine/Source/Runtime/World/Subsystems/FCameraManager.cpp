#include "pch.h"
#include "FCameraManager.h"

#include "World/World.h"

namespace Lumina
{
    SCameraComponent* FCameraManager::GetCameraComponent() const
    {
        CWorld* World = WeakWorld.Lock();
        if (World == nullptr)
        {
            return nullptr;
        }
        
        return World->GetEntityRegistry().try_get<SCameraComponent>(ActiveCameraEntity);
    }
}
