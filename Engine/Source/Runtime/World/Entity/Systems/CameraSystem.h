#pragma once
#include "EntitySystem.h"
#include "CameraSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SCameraSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(US_PostPhysics), RequiresUpdate(US_Paused))

        static void Startup(const FSystemContext& Context) noexcept;
        static void Update(const FSystemContext& Context) noexcept;
        static void Teardown(const FSystemContext& Context) noexcept;
    };
}
