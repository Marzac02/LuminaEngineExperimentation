#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"
#include "Containers/Tuple.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Variant/Variant.h"

namespace sol 
{
    template <>
    struct lua_type_of<Lumina::FString> : std::integral_constant<type, type::string> {};
    
    template <>
    struct is_container<Lumina::FString> : std::false_type {};
    
    template <typename T>
    struct unique_usertype_traits<Lumina::TObjectPtr<T>>
    {
        typedef T type;
        typedef Lumina::TObjectPtr<T> actual_type;
        static constexpr bool value = true;

        static bool is_null(const actual_type& ptr) 
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr) 
        {
            return ptr.Get();
        }
    };
    
}

namespace Lumina
{
    enum class EScriptType : uint8
    {
        WorldSystem,
        EntitySystem,
    };
    
    enum class EScriptVarTypes : uint8
    {
        Integer,
        Number,
        String,
        Bool,
        None,
    };
    
    template<typename T>
    struct TScriptVarTraits
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::None;
    };
    
    template<>
    struct TScriptVarTraits<int>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::Integer;
    };

    template<>
    struct TScriptVarTraits<float>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::Number;
    };

    template<>
    struct TScriptVarTraits<double>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::Number;
    };
    
    template<>
    struct TScriptVarTraits<FString>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::String;
    };
    
    template<>
    struct TScriptVarTraits<const char*>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::String;
    };
    
    template<>
    struct TScriptVarTraits<bool>
    {
        static constexpr EScriptVarTypes Type = EScriptVarTypes::Bool;
    };
    
    template<typename T>
    struct TNamedScriptVar
    {
        using Traits = TScriptVarTraits<T>;
        
        FName Name;
        T Value;
    };
    
    template<typename... Ts>
    using TScriptTuple = TTuple<TVector<TNamedScriptVar<Ts>>...>;
    
    using FScriptCustomData = TScriptTuple<bool, float, int, FString>;
    
    inline FArchive& operator << (FArchive& Ar, FScriptCustomData& Data)
    {
        auto SerializeVector = [&Ar]<typename T>(TVector<TNamedScriptVar<T>>& Vec)
        {
            if (Ar.IsWriting())
            {
                int32 Count = Vec.size();
                Ar << Count;
                
                for (auto& Var : Vec)
                {
                    Ar << Var.Name;
                    Ar << Var.Value;
                }
            }
            else if (Ar.IsReading())
            {
                int32 Count;
                Ar << Count;
                    
                Vec.resize(Count);
                for (int32 i = 0; i < Count; ++i)
                {
                    Ar << Vec[i].Name;
                    Ar << Vec[i].Value;
                }
            }
        };
        
        eastl::apply([&](auto&... Vectors)
        {
            (SerializeVector(Vectors), ...);
        }, Data);
        
        return Ar;
    }
    
}

namespace Lumina::Scripting
{
    struct FLuaScript
    {
        FName               Name;
        FString             Path;
        sol::environment    Environment;
        sol::table          ScriptTable;
    };
}
