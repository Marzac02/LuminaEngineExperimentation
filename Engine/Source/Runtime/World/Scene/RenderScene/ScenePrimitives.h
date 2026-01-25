#pragma once

#include "Containers/Array.h"
#include "glm/glm.hpp"


namespace Lumina
{
    struct SStaticMeshComponent;
    class FDeferredRenderScene;
    struct FMeshBatch;
    class CMaterialInterface;
    class CStaticMesh;
    
    struct FScenePrimitive
    {
        TVector<FMeshBatch> MeshBatches;
        glm::mat4           RenderTransform;
    };
    
    
}
