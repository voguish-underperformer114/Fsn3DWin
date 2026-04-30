#include "renderer/InstancedCubeRenderer.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <stdexcept>

namespace {
constexpr const char* InstancedCubeVertexShader = R"(
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aInstancePositionHighlight;
layout(location = 3) in vec4 aInstanceScaleCategory;
layout(location = 4) in vec4 aInstanceColor;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraPosition;

out vec3 vNormal;
out vec4 vColor;
out float vHighlight;
out float vCategory;
out float vDistance;

void main()
{
    vec3 worldPosition = aInstancePositionHighlight.xyz + aPosition * aInstanceScaleCategory.xyz;
    vNormal = normalize(aNormal);
    vColor = aInstanceColor;
    vHighlight = aInstancePositionHighlight.w;
    vCategory = aInstanceScaleCategory.w;
    vDistance = distance(worldPosition, uCameraPosition);
    gl_Position = uProjection * uView * vec4(worldPosition, 1.0);
}
)";

constexpr const char* InstancedCubeFragmentShader = R"(
#version 410 core

in vec3 vNormal;
in vec4 vColor;
in float vHighlight;
in float vCategory;
in float vDistance;

uniform float uAlpha;
uniform float uEmissionBoost;
uniform vec3 uFogColor;
uniform int uWireframe;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(vec3(-0.42, 0.85, 0.32));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float ambient = 0.24;
    float sideFalloff = 0.12 * max(normal.y, 0.0);
    float neon = max(max(vColor.r, vColor.g), vColor.b);
    float emission = 0.26 + max(neon - 0.58, 0.0) * 0.78 + vHighlight * 1.15 + uEmissionBoost;
    vec3 lit = vColor.rgb * (ambient + diffuse * 0.72 + sideFalloff) + vColor.rgb * emission;

    if (uWireframe == 1) {
        lit = vColor.rgb * (1.35 + vHighlight * 1.6 + uEmissionBoost);
    }

    float fog = smoothstep(42.0, 145.0, vDistance);
    lit = mix(lit, uFogColor, fog * 0.58);
    FragColor = vec4(lit, vColor.a * uAlpha * (1.0 - fog * 0.18));
}
)";
}

CubeInstance makeCubeInstance(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec4& color,
    float highlight,
    float categoryId)
{
    return {
        .positionHighlight = glm::vec4(position, highlight),
        .scaleCategory = glm::vec4(scale, categoryId),
        .color = color,
    };
}

const char* renderModeName(RenderMode mode)
{
    switch (mode) {
    case RenderMode::Solid:
        return "Solid";
    case RenderMode::Wireframe:
        return "Wireframe";
    case RenderMode::SolidWireframe:
    default:
        return "Solid + Wireframe";
    }
}

InstancedCubeRenderer::~InstancedCubeRenderer()
{
    shutdown();
}

void InstancedCubeRenderer::initialize()
{
    if (initialized_) {
        return;
    }

    cubeMesh_.createCube();
    shader_.loadFromSource(InstancedCubeVertexShader, InstancedCubeFragmentShader);

    glGenBuffers(1, &instanceVbo_);

    glBindVertexArray(cubeMesh_.vao());
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(CubeInstance)),
        reinterpret_cast<void*>(offsetof(CubeInstance, positionHighlight)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(CubeInstance)),
        reinterpret_cast<void*>(offsetof(CubeInstance, scaleCategory)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(CubeInstance)),
        reinterpret_cast<void*>(offsetof(CubeInstance, color)));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized_ = true;
}

void InstancedCubeRenderer::shutdown()
{
    if (instanceVbo_ != 0) {
        glDeleteBuffers(1, &instanceVbo_);
        instanceVbo_ = 0;
    }

    shader_ = Shader();
    cubeMesh_.shutdown();
    instanceCapacity_ = 0;
    lastInstanceCount_ = 0;
    initialized_ = false;
}

void InstancedCubeRenderer::render(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& fogColor,
    const std::vector<CubeInstance>& instances,
    RenderMode mode)
{
    lastInstanceCount_ = instances.size();
    if (!initialized_ || instances.empty()) {
        return;
    }

    uploadInstances(instances);

    shader_.use();
    shader_.setMat4("uView", view);
    shader_.setMat4("uProjection", projection);
    shader_.setVec3("uCameraPosition", cameraPosition);
    shader_.setVec3("uFogColor", fogColor);

    cubeMesh_.bind();

    if (mode == RenderMode::Solid || mode == RenderMode::SolidWireframe) {
        drawPass(false, 0.96f, 0.0f);
    }

    if (mode == RenderMode::Wireframe || mode == RenderMode::SolidWireframe) {
        drawPass(true, mode == RenderMode::Wireframe ? 0.88f : 0.42f, mode == RenderMode::Wireframe ? 0.42f : 0.28f);
    }

    glBindVertexArray(0);
}

std::size_t InstancedCubeRenderer::lastInstanceCount() const
{
    return lastInstanceCount_;
}

void InstancedCubeRenderer::uploadInstances(const std::vector<CubeInstance>& instances)
{
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);

    const std::size_t byteCount = instances.size() * sizeof(CubeInstance);
    if (instances.size() > instanceCapacity_) {
        instanceCapacity_ = instances.size();
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byteCount), instances.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(byteCount), instances.data());
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstancedCubeRenderer::drawPass(bool wireframe, float alpha, float emissionBoost)
{
    shader_.setFloat("uAlpha", alpha);
    shader_.setFloat("uEmissionBoost", emissionBoost);
    shader_.setInt("uWireframe", wireframe ? 1 : 0);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glLineWidth(1.8f);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glLineWidth(1.0f);
    }

    glDrawArraysInstanced(
        GL_TRIANGLES,
        0,
        cubeMesh_.vertexCount(),
        static_cast<GLsizei>(lastInstanceCount_));

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.0f);
    }
}
