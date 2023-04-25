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

    virtual PhysicsComponent AddCube(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos);

    virtual PhysicsComponent AddCorner(
        const glm::vec3 &size,
        const glm::vec3 &startPos,
        float startAngle,
        float endAngle);

    virtual PhysicsComponent AddRoad(
        const glm::vec3 &size,
        const glm::vec3 &startPos,
        bool isVertical);

    virtual PhysicsComponent AddSphere(
        float mass,
        float radius,
        const glm::vec3 &startPos);

    virtual CarComponent AddCar(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos,
        const float wheelRadius,
        const float wheelWidth);

    virtual void ApplyForce(
        const PhysicsComponent &component,
        const glm::vec3 &force,
        const glm::vec3 &relativePosition = glm::vec3());

    void ApplyEngineForce(
        const CarComponent &component,
        float force);

    virtual glm::mat4 GetMatrix(
        const PhysicsComponent &component);

    virtual glm::mat4 GetMatrix(
        const CarComponent &component);

    virtual glm::mat4 GetMatrix(
        const WheelComponent &component);

    virtual void RenderDebug(
        VertexArray &vertexAndColorBuffer);

private:
    btBroadphaseInterface *mBroadphase = nullptr;
    btDefaultCollisionConfiguration *mCollisionConfiguration = nullptr;
    btCollisionDispatcher *mDispatcher = nullptr;
    btSequentialImpulseConstraintSolver *mSolver = nullptr;
    btDiscreteDynamicsWorld *mDynamicsWorld = nullptr;
    std::vector<btRigidBody *> _rigidBodies;
    std::vector<btRaycastVehicle *> _vehicles;

    PhysicsComponent AddObject(
        btCollisionShape *shape,
        float mass,
        const glm::vec3 &startPos,
        bool allowDeactivate);

    bool SetupWheels(
        btRaycastVehicle *vehicle,
        const std::vector<btVector3> &wheelAxis,
        const btVector3 &wheelConnectionPoint,
        const float wheelRadius);
};

#endif // PHYSICSSERVICE_H
