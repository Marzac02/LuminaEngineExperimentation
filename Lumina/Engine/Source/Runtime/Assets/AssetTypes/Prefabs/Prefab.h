#pragma once
#include "Core/Object/Object.h"
#include "Prefab.generated.h"

namespace Lumina
{
    REFLECT()
    class LUMINA_API CPrefab : public CObject
    {
        GENERATED_BODY()
    public:
        
        void Serialize(FArchive& Ar) override;
        
        entt::registry Registry;
    };
}
