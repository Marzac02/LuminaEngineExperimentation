#include "pch.h"
#include "Prefab.h"
#include "World/Entity/EntityUtils.h"

namespace Lumina
{
    void CPrefab::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        ECS::Utils::SerializeRegistry(Ar, Registry);
    }
}
