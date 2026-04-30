#pragma once

#include "renderer/Mesh.hpp"
#include "renderer/Shader.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <vector>

enum class RenderMode {
    Solid = 0,
    Wireframe,
    SolidWireframe,
};

struct CubeInstance {
    glm::vec4 positionHighlight{};
    glm::vec4 scaleCategory{1.0f, 1.0f, 1.0f, 0.0f};
    glm::vec4 color{1.0f};
};

CubeInstance makeCubeInstance(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec4& color,
    float highlight,
    float categoryId);

const char* renderModeName(RenderMode mode);

class InstancedCubeRenderer {
public:
    ~InstancedCubeRenderer();

    InstancedCubeRenderer() = default;
    InstancedCubeRenderer(const InstancedCubeRenderer&) = delete;
    InstancedCubeRenderer& operator=(const InstancedCubeRenderer&) = delete;

    void initialize();
    void shutdown();
    void render(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& fogColor,
        const std::vector<CubeInstance>& instances,
        RenderMode mode);

    std::size_t lastInstanceCount() const;

private:
    void uploadInstances(const std::vector<CubeInstance>& instances);
    void drawPass(bool wireframe, float alpha, float emissionBoost);

    Mesh cubeMesh_;
    Shader shader_;
    unsigned int instanceVbo_ = 0;
    std::size_t instanceCapacity_ = 0;
    std::size_t lastInstanceCount_ = 0;
    bool initialized_ = false;
};
