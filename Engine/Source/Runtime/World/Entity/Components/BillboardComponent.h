#pragma once

#include "Core/Object/ObjectMacros.h"
#include "BillboardComponent.generated.h"

namespace Lumina
{
    class CTexture;
    
    REFLECT(EntityComponent)
    struct SBillboardComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable)
        TObjectPtr<CTexture> Texture;
    };
}
