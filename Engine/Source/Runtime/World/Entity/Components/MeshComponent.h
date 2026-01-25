#pragma once

#include "Assets/AssetTypes/Material/MaterialInterface.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "World/Scene/RenderScene/MeshBatch.h"
#include "MeshComponent.generated.h"

namespace Lumina
{
    class CMaterialInterface;
    
    REFLECT()
    struct RUNTIME_API SMeshComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Editable, Category = "Materials")
        TVector<TObjectPtr<CMaterialInterface>> MaterialOverrides;

    };
    
}
