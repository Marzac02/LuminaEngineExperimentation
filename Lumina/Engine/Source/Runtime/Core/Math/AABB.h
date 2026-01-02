#pragma once
#include "glm/glm.hpp"
#include "Platform/Platform.h"
#include "Core/Object/ObjectMacros.h"
#include "AABB.generated.h"


namespace Lumina
{
    REFLECT()
    struct FAABB
    {
        GENERATED_BODY()
        
        PROPERTY(Script, Editable)
        glm::vec3 Min;
        
        PROPERTY(Script, Editable)
        glm::vec3 Max;
        
        FAABB()
            : Min(0.0f), Max(0.0f)
        {}

        FAABB(const glm::vec3& InMin, const glm::vec3& InMax)
            : Min(InMin), Max(InMax)
        {}

        FUNCTION(Script)
        FORCEINLINE float MaxScale() const { return glm::max(GetSize().x, glm::max(GetSize().y, GetSize().z)); }
        
        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetSize() const { return Max - Min; }
        
        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetCenter() const { return Min + GetSize() * 0.5f; }

        NODISCARD FAABB ToWorld(const glm::mat4& World) const
        {
            glm::vec3 NewMin(eastl::numeric_limits<float>::max());
            glm::vec3 NewMax(-eastl::numeric_limits<float>::max());

            for (int i = 0; i < 8; i++)
            {
                glm::vec3 corner = glm::vec3
                (
                    (i & 1) ? Max.x : Min.x,
                    (i & 2) ? Max.y : Min.y,
                    (i & 4) ? Max.z : Min.z
                );

                glm::vec4 transformed = World * glm::vec4(corner, 1.0f);
                glm::vec3 p = glm::vec3(transformed) / transformed.w;

                NewMin = glm::min(NewMin, p);
                NewMax = glm::max(NewMax, p);
            }

            return FAABB(NewMin, NewMax);
        }
    };
}
