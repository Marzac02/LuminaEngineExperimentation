#pragma once
#include "Containers/Array.h"
#include "Core/Singleton/Singleton.h"
#include "Core/Threading/Thread.h"
#include "GUID/GUID.h"


namespace Lumina
{
    class CClass;
    class CPackage;
    class CObjectBase;
}

namespace Lumina
{

    using FObjectHashBucket = TFixedHashSet<CObjectBase*, 4>;

    template<typename TKey>
    using TObjectHashMap = TFixedHashMap<TKey, FObjectHashBucket, 12>;

    class FObjectHashTables : public TSingleton<FObjectHashTables>
    {
    public:

        void AddObject(CObjectBase* Object);

        void RemoveObject(CObjectBase* Object);

        CObjectBase* FindObject(const FGuid& GUID);
        CObjectBase* FindObject(const FName& Name, CClass* Class);

        void Clear();

        mutable FMutex                  Mutex;
        THashMap<FGuid, CObjectBase*>   ObjectGUIDHash;
        TObjectHashMap<CClass*>         ObjectClassBucket;
    };
}
