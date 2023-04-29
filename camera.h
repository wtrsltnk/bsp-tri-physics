#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera
{
public:
    Camera();
    virtual ~Camera();

    void ProcessMouseMovement(double delta_x, double delta_y, bool constraint_pitch);
    void update_target();

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

    glm::vec3 Target() const;

    void MoveForward(
        float amount);

    void MoveLeft(
        float amount);

    void MoveUp(
        float amount);

    float sensitivity = 0.05f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec4 target;

private:
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 _up = glm::vec3(0.0f, 0.0f, 1.0f);
};

#endif // CAMERA_H
