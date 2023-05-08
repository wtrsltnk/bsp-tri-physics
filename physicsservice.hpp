#ifndef PHYSICSSERVICE_H
#define PHYSICSSERVICE_H

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <LinearMath/btIDebugDraw.h>
#include <btBulletDynamicsCommon.h>
#include <chrono>
#include <glm/glm.hpp>
#include <iphysicsservice.hpp>
#include <vertexarray.hpp>

class PhysicsService : public IPhysicsService
{
public:
    PhysicsService();

    virtual ~PhysicsService();

    virtual void Step(
        std::chrono::microseconds diff);

    virtual PhysicsComponent AddStatic(
        const std::vector<glm::vec3> &t);

    virtual PhysicsComponent AddCharacter(
        float mass,
        float radius,
        float heigth,
        const glm::vec3 &startPos);

    virtual PhysicsComponent AddCube(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos);

    virtual PhysicsComponent AddSphere(
        float mass,
        float radius,
        const glm::vec3 &startPos);

    virtual void JumpCharacter(
        const PhysicsComponent &component,
        const glm::vec3 &direction);

    virtual void MoveCharacter(
        const PhysicsComponent &component,
        const glm::vec3 &direction,
        float speed);

    virtual void ApplyForce(
        const PhysicsComponent &component,
        const glm::vec3 &force,
        const glm::vec3 &relativePosition = glm::vec3());

    virtual glm::mat4 GetMatrix(
        const PhysicsComponent &component);

    virtual void RenderDebug(
        VertexArray &vertexAndColorBuffer);

private:
    btBroadphaseInterface *mBroadphase = nullptr;
    btDefaultCollisionConfiguration *mCollisionConfiguration = nullptr;
    btCollisionDispatcher *mDispatcher = nullptr;
    btSequentialImpulseConstraintSolver *mSolver = nullptr;
    btDiscreteDynamicsWorld *mDynamicsWorld = nullptr;
    std::vector<btRigidBody *> _rigidBodies;

    PhysicsComponent AddObject(
        btCollisionShape *shape,
        float mass,
        const glm::vec3 &startPos,
        bool allowDeactivate);
};

#endif // PHYSICSSERVICE_H
