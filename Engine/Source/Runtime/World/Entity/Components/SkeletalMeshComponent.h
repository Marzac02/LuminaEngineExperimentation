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
    struct RUNTIME_API SSkeletalMeshComponent : SMeshComponent
    {
        GENERATED_BODY()
        
        CMaterialInterface* GetMaterialForSlot(size_t Slot) const;
        
        FUNCTION(Script)
        FAABB GetAABB() const;
        
        PROPERTY(Editable, Category = "Mesh")
        TObjectPtr<CSkeletalMesh> SkeletalMesh;
        
        PROPERTY(Editable, Category = "Shadow")
        bool bCastShadow = true;

        PROPERTY(Editable, Category = "Shadow")
        bool bReceiveShadow = true;
        
        TArray<glm::mat4, 255> BoneTransforms;
    };
}
