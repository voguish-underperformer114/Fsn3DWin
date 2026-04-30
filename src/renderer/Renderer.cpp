#include "renderer/Renderer.hpp"

#include "scene/Camera.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {
struct SceneVertex {
    glm::vec3 position;
    glm::vec4 color;
};

constexpr const char* SceneVertexShader = R"(
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec4 uTint;

out vec4 vColor;

void main()
{
    vColor = aColor * uTint;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

constexpr const char* SceneFragmentShader = R"(
#version 410 core

in vec4 vColor;

uniform float uAlpha;
uniform float uIntensity;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor.rgb * uIntensity, vColor.a * uAlpha);
}
)";

void uploadVertices(unsigned int& vao, unsigned int& vbo, const std::vector<SceneVertex>& vertices)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(SceneVertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(SceneVertex)),
        reinterpret_cast<void*>(offsetof(SceneVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(SceneVertex)),
        reinterpret_cast<void*>(offsetof(SceneVertex, color)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void addLine(std::vector<SceneVertex>& vertices, glm::vec3 a, glm::vec3 b, glm::vec4 color)
{
    vertices.push_back({a, color});
    vertices.push_back({b, color});
}

glm::vec4 brightened(glm::vec4 color, float amount)
{
    color.r = std::min(color.r + amount, 1.0f);
    color.g = std::min(color.g + amount, 1.0f);
    color.b = std::min(color.b + amount, 1.0f);
    return color;
}
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::initialize()
{
    if (initialized_) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sceneShader_.loadFromSource(SceneVertexShader, SceneFragmentShader);
    cubeRenderer_.initialize();
    buildSceneGeometry();

    initialized_ = true;
}

void Renderer::shutdown()
{
    if (gridVbo_ != 0) {
        glDeleteBuffers(1, &gridVbo_);
        gridVbo_ = 0;
    }

    if (gridVao_ != 0) {
        glDeleteVertexArrays(1, &gridVao_);
        gridVao_ = 0;
    }

    cubeRenderer_.shutdown();
    sceneShader_ = Shader();
    gridVertexCount_ = 0;
    lastInstanceCount_ = 0;
    scanWaveInstances_.clear();
    sceneInstances_.clear();
    detailInstances_.clear();
    accentInstances_.clear();
    initialized_ = false;
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight, const glm::vec3& background)
{
    const int width = std::max(framebufferWidth, 1);
    const int height = std::max(framebufferHeight, 1);

    glViewport(0, 0, width, height);
    glClearColor(background.r, background.g, background.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderScene(
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
    bool scanActive)
{
    if (!initialized_) {
        return;
    }

    const DemoPalette palette = DemoScene::palette(theme);

    sceneShader_.use();
    sceneShader_.setMat4("uView", camera.viewMatrix());
    sceneShader_.setMat4("uProjection", camera.projectionMatrix(aspectRatio));

    const glm::mat4 identity(1.0f);
    const float gridPulse = 1.0f + 0.18f * std::sin(timeSeconds * 1.65f);

    sceneShader_.setMat4("uModel", identity);
    sceneShader_.setFloat("uAlpha", 1.0f);
    sceneShader_.setFloat("uIntensity", palette.gridIntensity * gridPulse);
    sceneShader_.setVec4("uTint", palette.gridTint);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.2f);
    glBindVertexArray(gridVao_);
    glDrawArrays(GL_LINES, 0, gridVertexCount_);

    scanWaveInstances_.clear();
    const float scanPosition = std::fmod(timeSeconds * (scanActive ? 17.5f : 8.0f), 78.0f) - 39.0f;
    scanWaveInstances_.push_back(
        makeCubeInstance(
            glm::vec3(0.0f, 0.065f, scanPosition),
            glm::vec3(80.0f, 0.035f, 0.55f),
            palette.scanWave,
            1.0f,
            0.0f));

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix(aspectRatio);

    cubeRenderer_.render(view, projection, camera.position(), palette.background, scanWaveInstances_, RenderMode::Solid);

    sceneInstances_.clear();
    detailInstances_.clear();
    accentInstances_.clear();

    if (sceneMode == SceneMode::Demo) {
        sceneInstances_.reserve(demoScene.blocks().size());
        detailInstances_.reserve(18);
        for (const DemoBlock& block : demoScene.blocks()) {
            const float pulse = 0.5f + 0.5f * std::sin(timeSeconds * 2.45f + block.pulsePhase);
            const float yPulse = 1.0f + block.pulseStrength * pulse * 0.055f;
            const glm::vec3 scale(block.scale.x, block.scale.y * yPulse, block.scale.z);
            const glm::vec4 color = DemoScene::colorFor(palette, block.category);
            const float scanBoost = scanActive && block.category == DemoBlockCategory::Root ? 1.15f : 0.0f;
            const float highlight = (block.important ? 0.65f : 0.0f) + block.pulseStrength * pulse * 0.65f + scanBoost;
            sceneInstances_.push_back(makeCubeInstance(
                block.position,
                scale,
                color,
                highlight,
                static_cast<float>(block.category)));

            if (block.important) {
                const float topY = block.position.y + scale.y * 0.5f;
                detailInstances_.push_back(makeCubeInstance(
                    glm::vec3(block.position.x, topY + 0.08f, block.position.z),
                    glm::vec3(scale.x * 0.72f, 0.16f, scale.z * 0.72f),
                    brightened(color, 0.18f),
                    0.82f + pulse * 0.45f,
                    static_cast<float>(block.category)));

                detailInstances_.push_back(makeCubeInstance(
                    glm::vec3(block.position.x, topY + 1.05f, block.position.z),
                    glm::vec3(0.08f, 1.85f + pulse * 0.35f, 0.08f),
                    glm::vec4(palette.scanWave.r, palette.scanWave.g, palette.scanWave.b, 1.0f),
                    1.05f,
                    static_cast<float>(block.category)));
            }
        }
    } else if (sceneNodes != nullptr) {
        constexpr std::size_t MaxDetailInstances = 512;
        sceneInstances_.reserve(sceneNodes->size());
        detailInstances_.reserve(std::min<std::size_t>(sceneNodes->size() / 4U + 8U, MaxDetailInstances));
        accentInstances_.reserve(8);

        for (const SceneNode& node : *sceneNodes) {
            if (!node.visible) {
                continue;
            }

            const float pulse = node.category == FileCategory::Directory
                ? 0.5f + 0.5f * std::sin(timeSeconds * 1.8f + static_cast<float>(node.sceneId) * 0.17f)
                : 0.0f;
            const bool important =
                node.category == FileCategory::Directory ||
                node.sceneId == 0 ||
                node.category == FileCategory::Executable;
            const bool selected = node.sceneId == selectedSceneId;
            const bool hovered = node.sceneId == hoveredSceneId && !selected;
            const float highlight = (important ? 0.42f : 0.0f) + pulse * 0.46f + (selected ? 1.55f : 0.0f) + (hovered ? 0.65f : 0.0f);
            glm::vec4 color = node.color;
            if (selected) {
                color = glm::vec4(
                    std::min(color.r + 0.42f, 1.0f),
                    std::min(color.g + 0.35f, 1.0f),
                    std::min(color.b + 0.18f, 1.0f),
                    color.a);
            } else if (hovered) {
                color = glm::vec4(
                    std::min(color.r + 0.16f, 1.0f),
                    std::min(color.g + 0.22f, 1.0f),
                    std::min(color.b + 0.28f, 1.0f),
                    color.a);
            }

            sceneInstances_.push_back(makeCubeInstance(
                node.position,
                node.scale,
                color,
                highlight,
                static_cast<float>(node.category)));

            if ((node.category == FileCategory::Directory || node.sceneId == 0 || node.category == FileCategory::Executable) &&
                (detailInstances_.size() < MaxDetailInstances || node.sceneId == 0 || selected)) {
                const float topY = node.position.y + node.scale.y * 0.5f;
                const glm::vec4 capColor = node.sceneId == 0 ? palette.root : brightened(node.color, 0.16f);
                const float capWidth = std::max(node.scale.x * 0.70f, 0.72f);
                detailInstances_.push_back(makeCubeInstance(
                    glm::vec3(node.position.x, topY + 0.07f, node.position.z),
                    glm::vec3(capWidth, 0.14f, capWidth),
                    capColor,
                    0.58f + pulse * 0.35f + (selected ? 0.45f : 0.0f),
                    static_cast<float>(node.category)));

                if (node.sceneId == 0 || selected || node.scale.y > 5.0f) {
                    detailInstances_.push_back(makeCubeInstance(
                        glm::vec3(node.position.x, topY + 0.95f, node.position.z),
                        glm::vec3(0.07f, 1.75f, 0.07f),
                        glm::vec4(palette.scanWave.r, palette.scanWave.g, palette.scanWave.b, 1.0f),
                        0.95f + pulse * 0.35f,
                        static_cast<float>(node.category)));
                }
            }

            if (selected || hovered) {
                const float markerScale = selected ? 2.25f : 1.75f;
                accentInstances_.push_back(makeCubeInstance(
                    glm::vec3(node.position.x, 0.055f, node.position.z),
                    glm::vec3(
                        std::max(node.scale.x * markerScale, selected ? 2.1f : 1.55f),
                        0.035f,
                        std::max(node.scale.z * markerScale, selected ? 2.1f : 1.55f)),
                    selected ? glm::vec4(1.0f, 0.96f, 0.42f, 0.92f) : glm::vec4(0.28f, 1.0f, 1.0f, 0.82f),
                    selected ? 1.15f : 0.68f,
                    static_cast<float>(node.category)));

                accentInstances_.push_back(makeCubeInstance(
                    glm::vec3(node.position.x, node.position.y + node.scale.y * 0.62f + (selected ? 2.25f : 1.35f), node.position.z),
                    glm::vec3(0.07f, selected ? 3.8f : 2.2f, 0.07f),
                    selected ? glm::vec4(1.0f, 0.96f, 0.42f, 0.95f) : glm::vec4(0.28f, 1.0f, 1.0f, 0.82f),
                    selected ? 1.35f : 0.75f,
                    static_cast<float>(node.category)));

                accentInstances_.push_back(makeCubeInstance(
                    node.position,
                    node.scale * glm::vec3(selected ? 1.18f : 1.10f),
                    selected ? glm::vec4(1.0f, 0.96f, 0.42f, 1.0f) : glm::vec4(0.28f, 1.0f, 1.0f, 1.0f),
                    selected ? 1.25f : 0.72f,
                    static_cast<float>(node.category)));
            }
        }
    }

    cubeRenderer_.render(view, projection, camera.position(), palette.background, sceneInstances_, renderMode);
    cubeRenderer_.render(view, projection, camera.position(), palette.background, detailInstances_, RenderMode::SolidWireframe);
    cubeRenderer_.render(view, projection, camera.position(), palette.background, accentInstances_, RenderMode::Wireframe);
    lastInstanceCount_ = sceneInstances_.size();
}

std::size_t Renderer::lastInstanceCount() const
{
    return lastInstanceCount_;
}

void Renderer::buildSceneGeometry()
{
    std::vector<SceneVertex> gridVertices;
    gridVertices.reserve(400);

    constexpr int gridExtent = 48;
    constexpr float gridY = 0.0f;

    for (int i = -gridExtent; i <= gridExtent; ++i) {
        if (i == 0) {
            continue;
        }

        const bool major = i % 5 == 0;
        const glm::vec4 color = major
            ? glm::vec4(0.0f, 0.95f, 1.0f, 0.62f)
            : glm::vec4(0.0f, 0.58f, 0.95f, 0.28f);

        const float offset = static_cast<float>(i);
        addLine(
            gridVertices,
            glm::vec3(-gridExtent, gridY, offset),
            glm::vec3(gridExtent, gridY, offset),
            color);
        addLine(
            gridVertices,
            glm::vec3(offset, gridY, -gridExtent),
            glm::vec3(offset, gridY, gridExtent),
            color);
    }

    addLine(gridVertices, glm::vec3(-gridExtent, 0.02f, 0.0f), glm::vec3(gridExtent, 0.02f, 0.0f), glm::vec4(1.0f, 0.12f, 0.22f, 0.95f));
    addLine(gridVertices, glm::vec3(0.0f, 0.02f, -gridExtent), glm::vec3(0.0f, 0.02f, gridExtent), glm::vec4(0.2f, 0.45f, 1.0f, 0.95f));
    addLine(gridVertices, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 8.0f, 0.0f), glm::vec4(0.22f, 1.0f, 0.35f, 0.95f));

    uploadVertices(gridVao_, gridVbo_, gridVertices);
    gridVertexCount_ = static_cast<int>(gridVertices.size());
}
