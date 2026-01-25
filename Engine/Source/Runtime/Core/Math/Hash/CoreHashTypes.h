#pragma once
#include "Hash.h"
#include <glm/glm.hpp>


namespace eastl
{

    template<typename T, glm::length_t L, glm::qualifier Q>
    struct hash<glm::vec<L, T, Q>>
    {
        size_t operator()(const glm::vec<L, T, Q>& v) const noexcept
        {
            size_t h = 0;
            for (glm::length_t i = 0; i < L; i++)
            {
                Lumina::Hash::HashCombine(h, v[i]);
            }
            return h;
        }
    };

    template<typename T, glm::length_t C, glm::length_t R, glm::qualifier Q>
    struct hash<glm::mat<C, R, T, Q>>
    {
        size_t operator()(const glm::mat<C, R, T, Q>& m) const noexcept
        {
            size_t h = 0;
            for (glm::length_t c = 0; c < C; c++)
            {
                for (glm::length_t r = 0; r < R; r++)
                {
                    Lumina::Hash::HashCombine(h, m[c][r]);
                }
            }
            return h;
        }
    };
}
