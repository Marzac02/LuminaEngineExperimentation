#include "pch.h"
#include "SkeletalMesh.h"

namespace Lumina
{
    void CSkeletalMesh::Serialize(FArchive& Ar)
    {
        CMesh::Serialize(Ar);
    }
}
