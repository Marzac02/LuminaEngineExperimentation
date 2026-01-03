#pragma once

#include "Component.h"
#include "Core/Object/ObjectMacros.h"
#include "HealthComponent.generated.h"

namespace Lumina
{
    REFLECT()
    struct LUMINA_API SHealthComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SHealthComponent)
        
        FUNCTION(Script)
        void ApplyDamage(float Damage) { Health -= Damage; }
        
        FUNCTION(Script)
        void GiveHealth(float NewHealth) { Health = glm::clamp(Health + NewHealth, 0.0f, MaxHealth); }
        
        PROPERTY(Script, Editable, Category = "Health")
        float Health = 100.0f;
        
        PROPERTY(Script, Editable, Category = "Health")
        float MaxHealth = 100.0f;
    };
}