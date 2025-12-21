#include "pch.h"
#include "StaticMeshComponent.h"

#include "assets/assettypes/material/MaterialInterface.h"



namespace Lumina
{
    CMaterialInterface* SStaticMeshComponent::GetMaterialForSlot(SIZE_T Slot) const
    {
        if (Slot < MaterialOverrides.size())
        {
            if (CMaterialInterface* Interface = MaterialOverrides[Slot])
            {
                return Interface;
            }
        }
        
        if (StaticMesh.IsValid())
        {
            return StaticMesh->GetMaterialAtSlot(Slot);
        }

        return nullptr;
    }

    FAABB SStaticMeshComponent::GetAABB() const
    {
        return StaticMesh ? StaticMesh->GetAABB() : FAABB();
    }
}
