#include "ReflectionDatabase.h"

namespace Lumina::Reflection
{
    void FReflectionDatabase::AddReflectedType(const eastl::shared_ptr<FReflectedType>& Type)
    {
        if(Type == nullptr || Type->DisplayName.empty())
        {
            return;
        }

        FStringHash NameHash = FStringHash(Type->QualifiedName);

        if (IsTypeRegistered(NameHash))
        {
            return;
        }
        
        auto& TypeVector = ReflectedTypes[Type->Header];
        TypeVector.push_back(Type);
        
        TypeHashMap.insert_or_assign(NameHash, Type);
    }

    bool FReflectionDatabase::IsTypeRegistered(const FStringHash& Str) const
    {
        return TypeHashMap.find(Str) != TypeHashMap.end() || IsCoreType(Str);
    }

    bool FReflectionDatabase::IsCoreType(const FStringHash& Hash) const
    {
        EPropertyTypeFlags Flags = GetCoreTypeFromName(Hash.c_str());

        return Flags != EPropertyTypeFlags::None;
    }
}
