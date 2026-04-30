#pragma once

#include "renderer/InstancedCubeRenderer.hpp"
#include "renderer/Shader.hpp"
#include "scene/DemoScene.hpp"
#include "scene/SceneNode.hpp"

#include <glm/vec3.hpp>

#include <vector>

class Camera;

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
        bool scanActive);

    std::size_t lastInstanceCount() const;

private:
    void buildSceneGeometry();

    Shader sceneShader_;
    InstancedCubeRenderer cubeRenderer_;
    std::vector<CubeInstance> scanWaveInstances_;
    std::vector<CubeInstance> sceneInstances_;
    std::vector<CubeInstance> detailInstances_;
    std::vector<CubeInstance> accentInstances_;
    unsigned int gridVao_ = 0;
    unsigned int gridVbo_ = 0;
    int gridVertexCount_ = 0;
    std::size_t lastInstanceCount_ = 0;
    bool initialized_ = false;
};
