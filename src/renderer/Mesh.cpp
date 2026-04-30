#include "renderer/Mesh.hpp"

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace {
struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

void addFace(
    std::vector<MeshVertex>& vertices,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c,
    glm::vec3 d,
    glm::vec3 normal)
{
    vertices.push_back({a, normal});
    vertices.push_back({b, normal});
    vertices.push_back({c, normal});
    vertices.push_back({c, normal});
    vertices.push_back({d, normal});
    vertices.push_back({a, normal});
}
}

Mesh::~Mesh()
{
    shutdown();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(std::exchange(other.vao_, 0))
    , vbo_(std::exchange(other.vbo_, 0))
    , vertexCount_(std::exchange(other.vertexCount_, 0))
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        shutdown();
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        vertexCount_ = std::exchange(other.vertexCount_, 0);
    }

    return *this;
}

void Mesh::createCube()
{
    shutdown();

    constexpr float h = 0.5f;
    const glm::vec3 p0(-h, -h, -h);
    const glm::vec3 p1(h, -h, -h);
    const glm::vec3 p2(h, h, -h);
    const glm::vec3 p3(-h, h, -h);
    const glm::vec3 p4(-h, -h, h);
    const glm::vec3 p5(h, -h, h);
    const glm::vec3 p6(h, h, h);
    const glm::vec3 p7(-h, h, h);

    std::vector<MeshVertex> vertices;
    vertices.reserve(36);
    addFace(vertices, p4, p5, p6, p7, glm::vec3(0.0f, 0.0f, 1.0f));
    addFace(vertices, p1, p0, p3, p2, glm::vec3(0.0f, 0.0f, -1.0f));
    addFace(vertices, p0, p4, p7, p3, glm::vec3(-1.0f, 0.0f, 0.0f));
    addFace(vertices, p5, p1, p2, p6, glm::vec3(1.0f, 0.0f, 0.0f));
    addFace(vertices, p3, p7, p6, p2, glm::vec3(0.0f, 1.0f, 0.0f));
    addFace(vertices, p0, p1, p5, p4, glm::vec3(0.0f, -1.0f, 0.0f));

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(MeshVertex)),
        reinterpret_cast<void*>(offsetof(MeshVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(MeshVertex)),
        reinterpret_cast<void*>(offsetof(MeshVertex, normal)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertexCount_ = static_cast<int>(vertices.size());
}

void Mesh::shutdown()
{
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    vertexCount_ = 0;
}

void Mesh::bind() const
{
    glBindVertexArray(vao_);
}

unsigned int Mesh::vao() const
{
    return vao_;
}

int Mesh::vertexCount() const
{
    return vertexCount_;
}
