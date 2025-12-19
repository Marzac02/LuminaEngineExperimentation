#pragma once
#include "Core/Templates/Optional.h"
#include "Ray/RayCast.h"


namespace Lumina::Physics
{
    class IPhysicsScene
    {
    public:

        virtual ~IPhysicsScene() { }
        virtual void PreUpdate() = 0;
        virtual void Update(double DeltaTime) = 0;
        virtual void PostUpdate() = 0;
        virtual void OnWorldSimulate() = 0;
        virtual void OnWorldStopSimulate() = 0;
        virtual TOptional<FRayResult> CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug = false, uint32 LayerMask = 0xFFFFFFFF, int64 IgnoreBody = -1) = 0;

        
    };
}
