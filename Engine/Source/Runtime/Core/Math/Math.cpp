#include "PCH.h"
#include "Math.h"
#include <glm/gtc/quaternion.hpp>


namespace Lumina::Math
{
    glm::quat FindLookAtRotation(const glm::vec3& Target, const glm::vec3& From)
    {
        glm::vec3 ForwardDirection = glm::normalize(Target - From);
        return glm::quatLookAt(ForwardDirection, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}
