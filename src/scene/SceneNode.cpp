#include "scene/SceneNode.hpp"

Aabb makeAabb(const glm::vec3& position, const glm::vec3& scale)
{
    const glm::vec3 halfScale = scale * 0.5f;
    return {
        .min = position - halfScale,
        .max = position + halfScale,
    };
}
