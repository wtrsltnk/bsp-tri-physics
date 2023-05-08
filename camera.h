#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera
{
public:
    Camera();
    virtual ~Camera();

    void ProcessMouseMovement(
        double delta_x,
        double delta_y,
        bool constraint_pitch);

    glm::mat4 GetViewMatrix();

    glm::vec3 Forward() const;

    glm::vec3 Back() const;

    glm::vec3 Left() const;

    glm::vec3 Right() const;

    const glm::vec3 &Up() const;

    void SetUp(
        const glm::vec3 &up);

    const glm::vec3 &Position() const;

    void SetPosition(
        const glm::vec3 &pos);

private:
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 _up = glm::vec3(0.0f, 0.0f, 1.0f);

    float sensitivity = 0.05f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec4 target;

    void update_target();
};

#endif // CAMERA_H
