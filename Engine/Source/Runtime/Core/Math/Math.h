#pragma once

#include <eastl/type_traits.h>
#include <random>
#include "eastl/utility.h"
#include <glm/glm.hpp>

namespace Lumina::Math
{
    template<typename T>
    constexpr T Max(const T& First, const T& Second)
    {
        return glm::max(First, Second);
    }

    template<typename T>
    constexpr T Min(const T& First, const T& Second)
    {
        return glm::min(First, Second);
    }

    template<typename T>
    constexpr T Clamp(const T& A, const T& First, const T& Second)
    {
        return glm::clamp(A,First,Second);
    }

    template<typename T>
    constexpr T Abs(const T& A)
    {
        return glm::abs(A);
    }

    template<typename T>
    constexpr T Floor(const T& A)
    {
        return glm::floor(A);
    }

    template<typename T>
    constexpr T Pow(const T& A, const T& B)
    {
        return glm::pow(A,B);
    }

    template<typename T>
    constexpr T Lerp(const T& A, const T& B, float Alpha)
    {
        return A + (B - A) * Alpha;
    }
    
    template<typename T>
    requires(eastl::is_integral_v<T> && eastl::is_unsigned_v<T> && (sizeof(T) <= 4))
    T RandRange(T First, T Second)
    {
        if (First > Second)
        {
            eastl::swap(First, Second);
        }
        
        thread_local std::mt19937 Random(std::random_device{}());
        std::uniform_int_distribution<T> Distribution(First, Second);
    
        return Distribution(Random);
    }

    RUNTIME_API glm::quat FindLookAtRotation(const glm::vec3& Target, const glm::vec3& From);
}
