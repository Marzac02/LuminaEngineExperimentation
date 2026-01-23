#pragma once

#include "StringHash.h"
#include "EASTL/hash_map.h"
#include "Reflector/Types/ReflectedType.h"

namespace Lumina::Reflection
{
    class FReflectionDatabase
    {
    public:

        FReflectionDatabase() = default;
        ~FReflectionDatabase() = default;

        void AddReflectedType(const eastl::shared_ptr<FReflectedType>& Type);

        bool IsTypeRegistered(const FStringHash& Str) const;

        bool IsCoreType(const FStringHash& Hash) const;
        
        template<typename T>
        requires(eastl::is_base_of_v<FReflectedType, T>)
        eastl::shared_ptr<T> GetOrCreateReflectedType(const FStringHash& TypeName);

        template<typename T>
        requires(eastl::is_base_of_v<FReflectedType, T>)
        eastl::shared_ptr<T> GetReflectedTypeChecked(const FStringHash& TypeName) const;

        template<typename T>
        eastl::shared_ptr<T> GetReflectedType(const FStringHash& TypeName) const;

        eastl::hash_map<FStringHash, eastl::vector<eastl::shared_ptr<FReflectedType>>>  ReflectedTypes;
        eastl::hash_map<FStringHash, eastl::shared_ptr<FReflectedType>>                 TypeHashMap;
        
    };


    //-------------------------------------------------------------------------------------

    template <typename T>
    requires(eastl::is_base_of_v<FReflectedType, T>)
    eastl::shared_ptr<T> FReflectionDatabase::GetOrCreateReflectedType(const FStringHash& TypeName)
    {
        eastl::shared_ptr<T> ReturnValue;
        if (IsTypeRegistered(TypeName))
        {
            ReturnValue = eastl::static_pointer_cast<T>(TypeHashMap.at(TypeName));
        }
        else
        {
            ReturnValue = eastl::make_shared<T>();
        }
        ReturnValue->QualifiedName = TypeName.c_str();

        return ReturnValue;
    }

    template <typename T> requires (eastl::is_base_of_v<FReflectedType, T>)
    eastl::shared_ptr<T> FReflectionDatabase::GetReflectedTypeChecked(const FStringHash& TypeName) const
    {
        if (!IsTypeRegistered(TypeName))
        {
            std::abort();
        }
        
        return eastl::static_pointer_cast<T>(TypeHashMap.at(TypeName));
    }

    template <typename T>
    eastl::shared_ptr<T> FReflectionDatabase::GetReflectedType(const FStringHash& TypeName) const
    {
        if (TypeHashMap.find(TypeName) == TypeHashMap.end())
        {
            return nullptr;
        }

        return eastl::static_pointer_cast<T>(TypeHashMap.at(TypeName));
    }
}
