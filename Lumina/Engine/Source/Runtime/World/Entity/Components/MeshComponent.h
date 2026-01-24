#pragma once

#include "Module/API.h"
#include "Assets/AssetTypes/Material/MaterialInterface.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "World/Scene/RenderScene/MeshBatch.h"
#include "MeshComponent.generated.h"

namespace Lumina
{
    class CMaterialInterface;
    
    REFLECT()
    struct LUMINA_API SMeshComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Category = "Materials")
        TVector<TObjectPtr<CMaterialInterface>> MaterialOverrides;

    };
    
}
