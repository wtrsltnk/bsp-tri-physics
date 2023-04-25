#include "physicsservice.hpp"

#include <entities.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

PhysicsService::PhysicsService()
{
    mCollisionConfiguration = new btDefaultCollisionConfiguration();
    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

    mBroadphase = new btDbvtBroadphase();

    mSolver = new btSequentialImpulseConstraintSolver();

    mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);
    mDynamicsWorld->setGravity(btVector3(0, 0, -9.81f));
    mDynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
}

PhysicsService::~PhysicsService() = default;

void PhysicsService::Step(
    std::chrono::microseconds diff)
{
    static double time = 0;
    time += static_cast<double>(diff.count() / 1000000000.0);
    while (time > 0)
    {
        mDynamicsWorld->stepSimulation(btScalar(1000.0f / 60.0f), 1, btScalar(1.0) / btScalar(30.0f));

        time -= (1000.0 / 60.0);
    }
}

PhysicsComponent PhysicsService::AddObject(
    btCollisionShape *shape,
    float mass,
    const glm::vec3 &startPos,
    bool disableDeactivation)
{
    btVector3 fallInertia(0, 0, 0);

    shape->calculateLocalInertia(mass, fallInertia);

    btRigidBody::btRigidBodyConstructionInfo rbci(mass, nullptr, shape, fallInertia);
    rbci.m_startWorldTransform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));

    auto body = new btRigidBody(rbci);
    if (disableDeactivation)
    {
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    auto bodyIndex = _rigidBodies.size();

    mDynamicsWorld->addRigidBody(body);
    _rigidBodies.push_back(body);

    return PhysicsComponent({
        bodyIndex,
    });
}

PhysicsComponent PhysicsService::AddCube(
    float mass,
    const glm::vec3 &size,
    const glm::vec3 &startPos)
{
    auto shape = new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));

    return AddObject(shape, mass, startPos, true);
}

PhysicsComponent PhysicsService::AddCorner(
    const glm::vec3 &size,
    const glm::vec3 &startPos,
    float startAngle,
    float endAngle)
{
    auto shape = new btCompoundShape();

    for (int i = startAngle; i < endAngle; i += 5)
    {
        auto box = new btBoxShape(btVector3(size.x * 0.1f, size.y * 0.1f, (size.z * 0.5f)));
        glm::mat4 m(1.0f);
        m = glm::rotate(m, glm::radians(float(i)), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::translate(m, glm::vec3(size.x * 0.37f, 0.0f, 0.0f));
        btTransform t;
        t.setFromOpenGLMatrix(glm::value_ptr(m));
        shape->addChildShape(t, box);
    }

    for (int i = startAngle; i <= endAngle; i += 10)
    {
        auto box = new btBoxShape(btVector3(size.x * 0.1f, size.y * 0.1f, (size.z * 0.5f)));
        glm::mat4 m(1.0f);
        m = glm::rotate(m, glm::radians(float(i)), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::translate(m, glm::vec3(size.x * 0.62f, 0.0f, 0.0f));
        btTransform t;
        t.setFromOpenGLMatrix(glm::value_ptr(m));
        shape->addChildShape(t, box);
    }

    return AddObject(shape, 0.0f, startPos, true);
}

PhysicsComponent PhysicsService::AddRoad(
    const glm::vec3 &size,
    const glm::vec3 &startPos,
    bool isVertical)
{
    auto shape = new btCompoundShape();

    float seperationScalar = 0.48f;
    if (isVertical)
    {
        auto boxl = new btBoxShape(btVector3((size.x * 0.5f) * seperationScalar, (size.y * 0.5f), (size.z * 0.5f)));
        btTransform tl;
        glm::mat4 ml(1.0f);
        ml = glm::translate(ml, glm::vec3((size.x * 0.5f) - size.x + ((size.x * 0.5f) * seperationScalar), 0.0f, 0.0f));
        tl.setFromOpenGLMatrix(glm::value_ptr(ml));
        shape->addChildShape(tl, boxl);

        auto boxr = new btBoxShape(btVector3((size.x * 0.5f) * seperationScalar, (size.y * 0.5f), (size.z * 0.5f)));
        btTransform tr;
        glm::mat4 mr(1.0f);
        mr = glm::translate(mr, glm::vec3((size.x * 0.5f) - ((size.x * 0.5f) * seperationScalar), 0.0f, 0.0f));
        tr.setFromOpenGLMatrix(glm::value_ptr(mr));
        shape->addChildShape(tr, boxr);
    }
    else
    {
        auto boxl = new btBoxShape(btVector3((size.x * 0.5f), (size.y * 0.5f) * seperationScalar, (size.z * 0.5f)));
        btTransform tl;
        glm::mat4 ml(1.0f);
        ml = glm::translate(ml, glm::vec3(0.0f, (size.y * 0.5f) - size.y + ((size.y * 0.5f) * seperationScalar), 0.0f));
        tl.setFromOpenGLMatrix(glm::value_ptr(ml));
        shape->addChildShape(tl, boxl);

        auto boxr = new btBoxShape(btVector3((size.x * 0.5f), (size.y * 0.5f) * seperationScalar, (size.z * 0.5f)));
        btTransform tr;
        glm::mat4 mr(1.0f);
        mr = glm::translate(mr, glm::vec3(0.0f, (size.y * 0.5f) - ((size.y * 0.5f) * seperationScalar), 0.0f));
        tr.setFromOpenGLMatrix(glm::value_ptr(mr));
        shape->addChildShape(tr, boxr);
    }

    return AddObject(shape, 0.0f, startPos, true);
}

PhysicsComponent PhysicsService::AddSphere(
    float mass,
    float radius,
    const glm::vec3 &startPos)
{
    auto shape = new btSphereShape(radius);

    return AddObject(shape, mass, startPos, true);
}

CarComponent PhysicsService::AddCar(
    float mass,
    const glm::vec3 &size,
    const glm::vec3 &startPos,
    const float wheelRadius,
    const float wheelWidth)
{
    btVector3 fallInertia(0, 0, 0);
    btTransform localTrans;
    btRaycastVehicle::btVehicleTuning tuning;

    //The direction of the raycast, the btRaycastVehicle uses raycasts instead of simiulating the wheels with rigid bodies
    btVector3 wheelDirectionCS0(0, 0, -1);

    //The axis which the wheel rotates arround
    btVector3 wheelAxleCS(1, 0, 0);

    btScalar connectionHeight(.2f);

    localTrans.setIdentity();
    localTrans.setOrigin(btVector3(0, 0, 0));

    btCollisionShape *chassis = new btBoxShape(btVector3(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f));
    btCollisionShape *pin = new btBoxShape(btVector3(0.2f, 0.2f, 2.0f));
    btCompoundShape *shape = new btCompoundShape();

    shape->addChildShape(localTrans, chassis);
    localTrans.setOrigin(btVector3(0, size.y / 2.0f, 0));
    btQuaternion quat;
    quat.setEuler(0.0f, 0.0f, glm::radians(45.0f));
    localTrans.setRotation(quat);
    shape->addChildShape(localTrans, pin);

    auto chassisComponent = AddObject(shape, mass, startPos, false);

    auto vehicleRayCaster = new btDefaultVehicleRaycaster(mDynamicsWorld);
    auto vehicle = new btRaycastVehicle(
        tuning,
        (btRigidBody *)mDynamicsWorld->getCollisionObjectArray()[chassisComponent.bodyIndex],
        vehicleRayCaster);

    btVector3 wheelConnectionPoint(
        (size.x / 2.0f) + wheelWidth,
        (size.y / 2.0f) - wheelRadius,
        -connectionHeight);

    vehicle->setCoordinateSystem(0, 1, 2);

    std::vector<btVector3> wheelAxis = {
        btVector3(1, 1, 1),
        btVector3(-1, 1, 1),
        btVector3(1, -1, 1),
        btVector3(-1, -1, 1),
    };

    auto result = SetupWheels(
        vehicle,
        wheelAxis,
        wheelConnectionPoint,
        wheelRadius);

    if (!result)
    {
        throw std::runtime_error("failed to add wheels");
    }

    mDynamicsWorld->addAction(vehicle);

    CarComponent carComponent({
        _vehicles.size(),
    });

    _vehicles.push_back(vehicle);

    return carComponent;
}

bool PhysicsService::SetupWheels(
    btRaycastVehicle *vehicle,
    const std::vector<btVector3> &wheelAxis,
    const btVector3 &wheelConnectionPoint,
    const float wheelRadius)
{
    if (vehicle == nullptr)
    {
        return false;
    }

    btRaycastVehicle::btVehicleTuning tuning;
    btVector3 wheelDirectionCS0(0, 0, -1);
    btVector3 wheelAxleCS(1, 0, 0);
    btScalar suspensionRestLength(0.6f);

    for (size_t i = 0; i < wheelAxis.size(); i++)
    {
        auto &wheel = vehicle->addWheel(
            wheelConnectionPoint * wheelAxis[i],
            wheelDirectionCS0,
            wheelAxleCS,
            suspensionRestLength,
            wheelRadius,
            tuning,
            false);

        wheel.m_suspensionStiffness = 20;
        wheel.m_wheelsDampingRelaxation = 1;
        wheel.m_wheelsDampingCompression = 0.8f;
        wheel.m_frictionSlip = 0.8f;
        wheel.m_rollInfluence = 1;
        wheel.m_rotation = 0;
        wheel.m_raycastInfo.m_suspensionLength = 0;
        wheel.m_worldTransform.setIdentity();
    }

    return vehicle->getNumWheels() == int(wheelAxis.size());
}

void PhysicsService::ApplyForce(
    const PhysicsComponent &component,
    const glm::vec3 &force,
    const glm::vec3 &relativePosition)
{
    _rigidBodies[component.bodyIndex]->applyForce(
        btVector3(force.x, force.y, force.z),
        btVector3(relativePosition.x, relativePosition.y, relativePosition.z));
}

void PhysicsService::ApplyEngineForce(
    const CarComponent &component,
    float force)
{
    _vehicles[component.carIndex]->applyEngineForce(force, 2);
    _vehicles[component.carIndex]->applyEngineForce(force, 3);
}

glm::mat4 PhysicsService::GetMatrix(
    const PhysicsComponent &component)
{
    glm::mat4 mat;

    _rigidBodies[component.bodyIndex]->getWorldTransform().getOpenGLMatrix(glm::value_ptr(mat));

    return mat;
}

glm::mat4 PhysicsService::GetMatrix(
    const CarComponent &component)
{
    auto vehicle = _vehicles[component.carIndex];

    glm::mat4 mat;

    vehicle->getRigidBody()->getWorldTransform().getOpenGLMatrix(glm::value_ptr(mat));

    return mat;
}

glm::mat4 PhysicsService::GetMatrix(
    const WheelComponent &component)
{
    auto vehicle = _vehicles[component.carIndex];

    if (component.wheelIndex >= vehicle->getNumWheels())
    {
        return glm::mat4(1.0f);
    }

    glm::mat4 mat;

    auto &wheelInfo = vehicle->getWheelInfo(component.wheelIndex);

    wheelInfo.m_worldTransform.getOpenGLMatrix(glm::value_ptr(mat));

    return mat;
}

class GLDebugDrawer : public btIDebugDraw
{
public:
    GLDebugDrawer(
        btDiscreteDynamicsWorld *dynamicsWorld,
        VertexArray &vertexAndColorBuffer);

    virtual ~GLDebugDrawer();

    virtual void drawLine(
        const btVector3 &from,
        const btVector3 &to,
        const btVector3 &color);

    virtual void drawContactPoint(
        const btVector3 &PointOnB,
        const btVector3 &normalOnB,
        btScalar distance,
        int lifeTime,
        const btVector3 &color);

    virtual void reportErrorWarning(
        const char *warningString);

    virtual void draw3dText(
        const btVector3 &location,
        const char *textString);

    virtual void setDebugMode(
        int debugMode);

    virtual int getDebugMode() const { return m_debugMode; }

private:
    int m_debugMode = 0;
    btDiscreteDynamicsWorld *_dynamicsWorld = nullptr;
    VertexArray &_vertexAndColorBuffer;
};

void PhysicsService::RenderDebug(
    VertexArray &vertexAndColorBuffer)
{
    GLDebugDrawer glDebugDrawer(
        mDynamicsWorld,
        vertexAndColorBuffer);

    mDynamicsWorld->debugDrawWorld();
}

#include <glad/glad.h>

GLDebugDrawer::GLDebugDrawer(
    btDiscreteDynamicsWorld *dynamicsWorld,
    VertexArray &vertexAndColorBuffer)
    : m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb),
      _dynamicsWorld(dynamicsWorld),
      _vertexAndColorBuffer(vertexAndColorBuffer)
{
    _dynamicsWorld->setDebugDrawer(this);
}

GLDebugDrawer::~GLDebugDrawer()
{
    _dynamicsWorld->setDebugDrawer(nullptr);
}

void GLDebugDrawer::drawLine(
    const btVector3 &from,
    const btVector3 &to,
    const btVector3 &color)
{
    float lineData[] = {
        from.getX(),
        from.getY(),
        from.getZ(),
        color.getX(),
        color.getY(),
        color.getZ(),
        to.getX(),
        to.getY(),
        to.getZ(),
        color.getX(),
        color.getY(),
        color.getZ(),
    };

    _vertexAndColorBuffer.add(lineData, 2);
}

void GLDebugDrawer::setDebugMode(
    int debugMode)
{
    m_debugMode = debugMode;
}

void GLDebugDrawer::draw3dText(
    const btVector3 &,
    const char *)
{}

void GLDebugDrawer::reportErrorWarning(
    const char *warningString)
{
    printf("%s", warningString);
}

void GLDebugDrawer::drawContactPoint(
    const btVector3 &,
    const btVector3 &,
    btScalar,
    int,
    const btVector3 &)
{}
