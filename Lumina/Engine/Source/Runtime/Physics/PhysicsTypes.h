#pragma once

namespace Lumina
{
    enum class EMoveMode
    {
        Teleport,           // Hard set position (loses velocity)
        MoveKinematic,      // Move with velocity calculation (preserves physics)
        ActivateOnly        // Just wake up, don't move
    };
}