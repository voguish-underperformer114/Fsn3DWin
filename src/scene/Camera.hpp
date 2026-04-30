#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera();

    void reset();
    void move(float forward, float right, float up, float deltaSeconds);
    void look(float deltaX, float deltaY);
    void lookAt(const glm::vec3& target);
    void focusOn(const glm::vec3& target, float radius);
    void setPosition(const glm::vec3& position);
    void changeMovementSpeed(float wheelDelta);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    const glm::vec3& position() const;
    float yaw() const;
    float pitch() const;
    float movementSpeed() const;

private:
    glm::vec3 front() const;
    glm::vec3 right() const;

    glm::vec3 position_{};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float fovDegrees_ = 60.0f;
    float movementSpeed_ = 8.0f;
};
