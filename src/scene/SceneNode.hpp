#pragma once

#include "filesystem/FileNode.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>

using SceneNodeId = std::uint32_t;
constexpr SceneNodeId InvalidSceneNodeId = UINT32_MAX;

struct Aabb {
    glm::vec3 min{};
    glm::vec3 max{};
};

struct SceneNode {
    SceneNodeId sceneId = InvalidSceneNodeId;
    SceneNodeId parentSceneId = InvalidSceneNodeId;
    FileNodeId sourceFileNodeId = InvalidFileNodeId;
    glm::vec3 position{};
    glm::vec3 scale{1.0f};
    glm::vec4 color{1.0f};
    FileCategory category = FileCategory::Other;
    std::string label;
    Aabb bounds{};
    bool visible = true;
};

Aabb makeAabb(const glm::vec3& position, const glm::vec3& scale);
