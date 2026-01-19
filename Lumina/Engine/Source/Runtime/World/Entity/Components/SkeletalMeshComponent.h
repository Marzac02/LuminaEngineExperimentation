#pragma once
#include "meshcomponent.h"
#include "Core/Math/AABB.h"
#include "Core/Object/ObjectMacros.h"
#include "SkeletalMeshComponent.generated.h"


namespace Lumina
{
    class CMaterialInterface;
    class CSkeletalMesh;
    
    
    REFLECT(Component)
    struct LUMINA_API SSkeletalMeshComponent : SMeshComponent
    {
        GENERATED_BODY()
        
        CMaterialInterface* GetMaterialForSlot(SIZE_T Slot) const;
        
        FUNCTION(Script)
        FAABB GetAABB() const;
        
        PROPERTY(Editable, Category = "Mesh")
        TObjectPtr<CSkeletalMesh> SkeletalMesh;
        
        TArray<glm::mat4, 255> BoneTransforms;
    };
}
