#include "pch.h"
#include "Prefab.h"

#include "World/Entity/EntityUtils.h"
#include "World/Entity/Components/Component.h"


namespace Lumina
{
    void CPrefab::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        ECS::Utils::SerializeRegistry(Ar, Registry);
    }
}
