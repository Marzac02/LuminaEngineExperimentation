#pragma once

#include "Core/Object/ObjectMacros.h"
#include "CharacterControllerComponent.generated.h"


namespace Lumina
{
    REFLECT(Component)
    struct LUMINA_API SCharacterControllerComponent
    {
        GENERATED_BODY()
        
        FUNCTION(Script)
        FORCEINLINE void AddMovementInput(const glm::vec3& Move) { MoveInput += Move; }
        
        FUNCTION(Script)
        FORCEINLINE void AddLookInput(const glm::vec2& Look) { LookInput += Look; }
        
        FUNCTION(Script)
        FORCEINLINE void Jump() { bJumpPressed = true; }
        
        
        PROPERTY(Script, ReadOnly)
        glm::vec3 MoveInput;

        PROPERTY(Script, ReadOnly)
        glm::vec2 LookInput;

        PROPERTY(Script, ReadOnly)
        bool bJumpPressed = false;
    };
}
