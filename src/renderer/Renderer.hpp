#pragma once

#include "renderer/InstancedCubeRenderer.hpp"
#include "renderer/Shader.hpp"
#include "scene/DemoScene.hpp"
#include "scene/SceneNode.hpp"

#include <glm/vec3.hpp>

#include <vector>

class Camera;

struct ImageBillboard {
    unsigned int textureId = 0;
    glm::vec3 position{};
    float width = 1.0f;
    float height = 1.0f;
};

class Renderer {
public:
    ~Renderer();

    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void initialize();
    void shutdown();
    void beginFrame(int framebufferWidth, int framebufferHeight, const glm::vec3& background);
    void renderScene(
        const Camera& camera,
        float aspectRatio,
        float timeSeconds,
        const DemoScene& demoScene,
        DemoTheme theme,
        SceneMode sceneMode,
        const std::vector<SceneNode>* sceneNodes,
        RenderMode renderMode,
        SceneNodeId selectedSceneId,
        SceneNodeId hoveredSceneId,
        bool scanActive,
        const ImageBillboard* imageBillboard);

    std::size_t lastInstanceCount() const;

private:
    void buildSceneGeometry();
    void buildImageGeometry();
    void buildLinkGeometry();
    void renderHierarchyLinks(
        const Camera& camera,
        float aspectRatio,
        const std::vector<SceneNode>& sceneNodes,
        const glm::vec4& tint);
    void renderImageBillboard(const Camera& camera, float aspectRatio, const ImageBillboard& billboard);

    Shader sceneShader_;
    Shader imageShader_;
    InstancedCubeRenderer cubeRenderer_;
    std::vector<CubeInstance> scanWaveInstances_;
    std::vector<CubeInstance> sceneInstances_;
    std::vector<CubeInstance> detailInstances_;
    std::vector<CubeInstance> accentInstances_;
    unsigned int gridVao_ = 0;
    unsigned int gridVbo_ = 0;
    unsigned int imageVao_ = 0;
    unsigned int imageVbo_ = 0;
    unsigned int linkVao_ = 0;
    unsigned int linkVbo_ = 0;
    int gridVertexCount_ = 0;
    int imageVertexCount_ = 0;
    std::size_t linkVertexCapacity_ = 0;
    std::size_t lastInstanceCount_ = 0;
    bool initialized_ = false;
};
