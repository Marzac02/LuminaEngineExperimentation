#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Component.h"
#include "CharacterControllerComponent.generated.h"


namespace Lumina
{
    REFLECT()
    struct LUMINA_API SCharacterControllerComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SCharacterControllerComponent)
        
        PROPERTY(Script)
        glm::vec2 MoveInput;

        PROPERTY(Script)
        glm::vec2 LookInput;

        PROPERTY(Script)
        bool bJumpPressed = false;

        PROPERTY(Script, Editable)
        bool bUseControllerRotation = false;
        
    };
}
