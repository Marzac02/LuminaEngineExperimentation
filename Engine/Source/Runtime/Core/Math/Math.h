#pragma once
#include <eastl/type_traits.h>
#include <random>
#include "Platform/Platform.h"
#include "eastl/utility.h"
#include <glm/glm.hpp>

namespace Lumina::Math
{
    template<typename T>
    FORCEINLINE T Max(const T& First, const T& Second)
    {
        return glm::max(First, Second);
    }

    template<typename T>
    FORCEINLINE T Min(const T& First, const T& Second)
    {
        return glm::min(First, Second);
    }

    template<typename T>
    FORCEINLINE T Clamp(const T& A, const T& First, const T& Second)
    {
        return glm::clamp(A,First,Second);
    }

    template<typename T>
    FORCEINLINE T Abs(const T& A)
    {
        return glm::abs(A);
    }

    template<typename T>
    FORCEINLINE T Floor(const T& A)
    {
        return glm::floor(A);
    }

    template<typename T>
    FORCEINLINE T Pow(const T& A, const T& B)
    {
        return glm::pow(A,B);
    }

    template<typename T>
    FORCEINLINE T Lerp(const T& A, const T& B, float Alpha)
    {
        return A + (B - A) * Alpha;
    }
    
    template<typename T>
    requires(eastl::is_integral_v<T> && eastl::is_unsigned_v<T> && (sizeof(T) <= 4))
    FORCEINLINE T RandRange(T First, T Second)
    {
        if (First > Second)
        {
            eastl::swap(First, Second);
        }
        
        thread_local std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<T> dist(First, Second);
    
        return dist(rng);
    }

    RUNTIME_API glm::quat FindLookAtRotation(const glm::vec3& Target, const glm::vec3& From);
}
