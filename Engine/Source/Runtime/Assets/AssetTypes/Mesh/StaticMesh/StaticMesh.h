#pragma once
#include "Assets/AssetTypes/Mesh/Mesh.h"
#include "StaticMesh.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CStaticMesh : public CMesh
    {
        GENERATED_BODY()
        
    public:

        bool IsAsset() const override { return true; }

    };
}
