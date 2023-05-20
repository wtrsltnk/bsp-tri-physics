#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

Camera::Camera()
{
    update_target();
}

Camera::~Camera() = default;

void Camera::ProcessMouseMovement(
    float delta_x,
    float delta_y,
    bool constraint_pitch = true)
{
    yaw -= delta_x * sensitivity;
    pitch += delta_y * sensitivity;

    if (constraint_pitch)
    {
        if (pitch > 89.0f)
        {
            pitch = 89.0f;
        }
        if (pitch < -89.0f)
        {
            pitch = -89.0f;
        }
    }
    update_target();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(this->Position(), this->Position() + glm::vec3(target), this->Up());
}

glm::vec3 Camera::Forward() const
{
    return target;
}

glm::vec3 Camera::Back() const
{
    return -1.0f * Forward();
}

glm::vec3 Camera::Left() const
{
    return glm::cross(this->Up(), this->Forward());
}

glm::vec3 Camera::Right() const
{
    return -1.0f * Left();
}

const glm::vec3 &Camera::Up() const
{
    return this->_up;
}

void Camera::SetUp(
    const glm::vec3 &up)
{
    this->_up = up;
}

const glm::vec3 &Camera::Position() const
{
    return this->_position;
}

void Camera::SetPosition(
    const glm::vec3 &pos)
{
    this->_position = pos;
}

void Camera::update_target()
{
    float yaw_radians = glm::radians(yaw);
    float pitch_radians = glm::radians(pitch);

    target.x = -sin(yaw_radians) * cos(pitch_radians);
    target.y = -cos(yaw_radians) * cos(pitch_radians);
    target.z = sin(pitch_radians);
}
