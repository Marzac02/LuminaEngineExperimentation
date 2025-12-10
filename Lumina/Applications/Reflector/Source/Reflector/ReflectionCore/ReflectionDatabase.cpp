#include "ReflectionDatabase.h"


namespace Lumina::Reflection
{
    FReflectionDatabase::~FReflectionDatabase()
    {
        
    }

    void FReflectionDatabase::AddReflectedProject(const eastl::shared_ptr<FReflectedProject>& Project)
    {
        ReflectedProjects.push_back(Project);
    }

    void FReflectionDatabase::AddReflectedType(const eastl::shared_ptr<FReflectedType>& Type)
    {
        if(Type == nullptr || Type->DisplayName.empty())
        {
            return;
        }

        FStringHash NameHash = FStringHash(Type->QualifiedName);
        FStringHash PathHash = FStringHash(Type->HeaderID);

        if (IsTypeRegistered(NameHash))
        {
            return;
        }
        
        eastl::vector<eastl::shared_ptr<FReflectedType>>* TypeVector = &ReflectedTypes[PathHash];
        TypeVector->push_back(Type);
        
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
