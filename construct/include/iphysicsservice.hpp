#ifndef IPHYSICSSERVICE_H
#define IPHYSICSSERVICE_H

#include <chrono>
#include <entities.hpp>
#include <glm/glm.hpp>
#include <tuple>
#include <vector>
#include <vertexarray.hpp>

class IPhysicsService
{
public:
    virtual ~IPhysicsService() = default;

    virtual void Step(
        std::chrono::microseconds diff) = 0;

    virtual PhysicsComponent AddStatic(
        const std::vector<glm::vec3> &t) = 0;

    virtual PhysicsComponent AddCharacter(
        float mass,
        float radius,
        float heigth,
        const glm::vec3 &startPos) = 0;

    virtual PhysicsComponent AddCube(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos) = 0;

    virtual PhysicsComponent AddSphere(
        float mass,
        float radius,
        const glm::vec3 &startPos) = 0;

    virtual void JumpCharacter(
        const PhysicsComponent &component,
        const glm::vec3 &direction) = 0;

    virtual void MoveCharacter(
        const PhysicsComponent &component,
        const glm::vec3 &direction,
        float speed) = 0;

    virtual void ApplyForce(
        const PhysicsComponent &component,
        const glm::vec3 &force,
        const glm::vec3 &relativePosition = glm::vec3()) = 0;

    virtual glm::mat4 GetMatrix(
        const PhysicsComponent &component) = 0;

    virtual void RenderDebug(
        VertexArray &vertexAndColorBuffer) = 0;
};

#endif // IPHYSICSSERVICE_H
