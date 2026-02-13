#pragma once
#include "EntitySystem.h"
#include "ScriptSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SScriptSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(
              RequiresUpdate(EUpdateStage::FrameStart)
            , RequiresUpdate(EUpdateStage::PrePhysics)
            , RequiresUpdate(EUpdateStage::DuringPhysics)
            , RequiresUpdate(EUpdateStage::PostPhysics)
            , RequiresUpdate(EUpdateStage::FrameEnd))

        static void Startup(const FSystemContext& Context) noexcept;
        static void Update(const FSystemContext& Context) noexcept;
        static void Teardown(const FSystemContext& Context) noexcept;
    };
}
