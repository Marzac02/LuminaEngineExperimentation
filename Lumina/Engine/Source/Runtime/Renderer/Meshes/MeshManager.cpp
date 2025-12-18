#include "pch.h"
#include "MeshManager.h"


namespace Lumina
{
    FMeshManager& FMeshManager::Get()
    {
        static FMeshManager Instance;
        return Instance;
    }
}
