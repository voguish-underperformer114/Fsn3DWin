#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void loadFromSource(const char* vertexSource, const char* fragmentSource);
    void use() const;

    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;
    void setMat4(const char* name, const glm::mat4& value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setVec4(const char* name, const glm::vec4& value) const;

private:
    void destroy();
    static unsigned int compileShader(unsigned int type, const char* source);

    unsigned int program_ = 0;
};
