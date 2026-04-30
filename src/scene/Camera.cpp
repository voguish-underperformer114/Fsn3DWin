#include "scene/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace {
constexpr glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};
constexpr float MouseSensitivity = 0.11f;
constexpr float MinPitch = -86.0f;
constexpr float MaxPitch = 86.0f;
constexpr float MinSpeed = 1.0f;
constexpr float MaxSpeed = 60.0f;
}

Camera::Camera()
{
    reset();
}

void Camera::reset()
{
    position_ = glm::vec3(18.0f, 12.0f, 24.0f);
    yaw_ = -137.0f;
    pitch_ = -29.0f;
    fovDegrees_ = 60.0f;
    movementSpeed_ = 10.0f;
}

void Camera::move(float forwardAmount, float rightAmount, float upAmount, float deltaSeconds)
{
    const float distance = movementSpeed_ * deltaSeconds;
    position_ += front() * forwardAmount * distance;
    position_ += right() * rightAmount * distance;
    position_ += WorldUp * upAmount * distance;
}

void Camera::look(float deltaX, float deltaY)
{
    yaw_ += deltaX * MouseSensitivity;
    pitch_ -= deltaY * MouseSensitivity;
    pitch_ = std::clamp(pitch_, MinPitch, MaxPitch);
}

void Camera::lookAt(const glm::vec3& target)
{
    const glm::vec3 direction = glm::normalize(target - position_);
    yaw_ = glm::degrees(std::atan2(direction.z, direction.x));
    pitch_ = glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f)));
    pitch_ = std::clamp(pitch_, MinPitch, MaxPitch);
}

void Camera::focusOn(const glm::vec3& target, float radius)
{
    const float distance = std::clamp(radius * 4.0f + 5.0f, 6.0f, 90.0f);
    glm::vec3 viewDirection = front();
    if (glm::length(viewDirection) < 0.001f) {
        viewDirection = glm::normalize(glm::vec3(-1.0f, -0.35f, -1.0f));
    }

    position_ = target - viewDirection * distance + WorldUp * std::min(radius * 0.35f, 4.0f);
    lookAt(target);
}

void Camera::setPosition(const glm::vec3& position)
{
    position_ = position;
}

void Camera::changeMovementSpeed(float wheelDelta)
{
    const float multiplier = std::pow(1.18f, wheelDelta);
    movementSpeed_ = std::clamp(movementSpeed_ * multiplier, MinSpeed, MaxSpeed);
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position_, position_ + front(), WorldUp);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fovDegrees_), aspectRatio, 0.1f, 500.0f);
}

const glm::vec3& Camera::position() const
{
    return position_;
}

float Camera::yaw() const
{
    return yaw_;
}

float Camera::pitch() const
{
    return pitch_;
}

float Camera::movementSpeed() const
{
    return movementSpeed_;
}

glm::vec3 Camera::front() const
{
    const float yawRadians = glm::radians(yaw_);
    const float pitchRadians = glm::radians(pitch_);

    return glm::normalize(glm::vec3(
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians)));
}

glm::vec3 Camera::right() const
{
    return glm::normalize(glm::cross(front(), WorldUp));
}
