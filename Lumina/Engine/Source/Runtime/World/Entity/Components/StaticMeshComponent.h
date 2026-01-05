#pragma once


#include "MeshComponent.h"
#include "Core/Math/Transform.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "StaticMeshComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct LUMINA_API SStaticMeshComponent : SMeshComponent
    {
        GENERATED_BODY()
        
        CMaterialInterface* GetMaterialForSlot(SIZE_T Slot) const;
        
        FUNCTION(Script)
        FAABB GetAABB() const;
        
        PROPERTY(Editable, Category = "Mesh")
        TObjectPtr<CStaticMesh> StaticMesh;

        TVector<FTransform>        Instances;
    };
}
