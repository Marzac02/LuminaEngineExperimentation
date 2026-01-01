#include "PCH.h"
#include "Math.h"
#include <glm/gtc/quaternion.hpp>


namespace Lumina::Math
{
    glm::quat FindLookAtRotation(const glm::vec3& From, const glm::vec3& To)
    {
        glm::vec3 ForwardDirection = glm::normalize(To - From);
        return glm::quatLookAt(ForwardDirection, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}
