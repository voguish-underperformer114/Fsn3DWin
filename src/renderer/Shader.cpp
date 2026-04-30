#include "renderer/Shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
std::string shaderInfoLog(unsigned int shader)
{
    int length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length <= 1) {
        return {};
    }

    std::vector<char> buffer(static_cast<size_t>(length));
    glGetShaderInfoLog(shader, length, nullptr, buffer.data());
    return std::string(buffer.data());
}

std::string programInfoLog(unsigned int program)
{
    int length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    if (length <= 1) {
        return {};
    }

    std::vector<char> buffer(static_cast<size_t>(length));
    glGetProgramInfoLog(program, length, nullptr, buffer.data());
    return std::string(buffer.data());
}
}

Shader::~Shader()
{
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : program_(std::exchange(other.program_, 0))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        destroy();
        program_ = std::exchange(other.program_, 0);
    }

    return *this;
}

void Shader::loadFromSource(const char* vertexSource, const char* fragmentSource)
{
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (linked != GL_TRUE) {
        const std::string log = programInfoLog(program);
        glDeleteProgram(program);
        std::cerr << "Shader link error:\n" << log << '\n';
        throw std::runtime_error("Failed to link shader program");
    }

    destroy();
    program_ = program;
}

void Shader::use() const
{
    glUseProgram(program_);
}

void Shader::setFloat(const char* name, float value) const
{
    glUniform1f(glGetUniformLocation(program_, name), value);
}

void Shader::setInt(const char* name, int value) const
{
    glUniform1i(glGetUniformLocation(program_, name), value);
}

void Shader::setMat4(const char* name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const char* name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(value));
}

void Shader::destroy()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

unsigned int Shader::compileShader(unsigned int type, const char* source)
{
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled != GL_TRUE) {
        const std::string log = shaderInfoLog(shader);
        glDeleteShader(shader);
        std::cerr << "Shader compile error:\n" << log << '\n';
        throw std::runtime_error("Failed to compile shader");
    }

    return shader;
}
