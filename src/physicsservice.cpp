#include "physicsservice.hpp"

#include <entities.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>

const btScalar scalef = 0.08f;

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

btRigidBody *CreateBody(
    btCollisionShape *shape,
    float mass,
    const glm::vec3 &startPos,
    const glm::vec3 &startRot,
    bool disableDeactivation)
{
    btVector3 fallInertia(0, 0, 0);

    if (mass > 0.0f)
    {
        shape->calculateLocalInertia(mass, fallInertia);
    }

    btQuaternion quat;
    quat.setEulerZYX(startRot.x, startRot.y, startRot.z);

    btRigidBody::btRigidBodyConstructionInfo rbci(mass, nullptr, shape, fallInertia);
    rbci.m_startWorldTransform.setOrigin(btVector3(startPos.x * scalef, startPos.y * scalef, startPos.z * scalef));
    rbci.m_startWorldTransform.setRotation(quat);

    auto body = new btRigidBody(rbci);
    if (disableDeactivation)
    {
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    return body;
}

PhysicsComponent PhysicsService::AddObject(
    btCollisionShape *shape,
    float mass,
    const glm::vec3 &startPos,
    bool disableDeactivation)
{
    auto body = CreateBody(shape, mass, startPos, glm::vec3(0.0f), disableDeactivation);

    auto bodyIndex = _rigidBodies.size();

    mDynamicsWorld->addRigidBody(body);
    _rigidBodies.push_back(body);

    return PhysicsComponent({
        bodyIndex,
    });
}

PhysicsComponent PhysicsService::AddCharacter(
    float mass,
    float radius,
    float height,
    const glm::vec3 &startPos)
{
    auto shape = new btCapsuleShape(radius * scalef, height * scalef);

    auto body = CreateBody(shape, mass, startPos, glm::vec3(0.0f, 0.0f, glm::radians(90.0f)), true);

    /// Make sure this object is always busy
    body->setSleepingThresholds(0.0f, 0.0f);

    /// Make sure the capule will stay up
    body->setAngularFactor(0.0f);

    /// No to almost no bouncing on the floor
    body->setRestitution(0.0f);

    /// No friction to make sure you can't "climb walls"
    body->setFriction(0.0f);

    auto bodyIndex = _rigidBodies.size();

    mDynamicsWorld->addRigidBody(body);
    _rigidBodies.push_back(body);

    return PhysicsComponent({
        bodyIndex,
    });
}

PhysicsComponent PhysicsService::AddStatic(
    const std::vector<glm::vec3> &t)
{
    auto mesh = new btTriangleMesh();

    for (size_t i = 0; i < t.size(); i += 3)
    {
        mesh->addTriangle(
            btVector3(t[i + 0].x * scalef, t[i + 0].y * scalef, t[i + 0].z * scalef),
            btVector3(t[i + 1].x * scalef, t[i + 1].y * scalef, t[i + 1].z * scalef),
            btVector3(t[i + 2].x * scalef, t[i + 2].y * scalef, t[i + 2].z * scalef));
    }

    auto shape = new btBvhTriangleMeshShape(mesh, true);

    return AddObject(shape, 0.0f, glm::vec3(), false);
}

PhysicsComponent PhysicsService::AddCube(
    float mass,
    const glm::vec3 &size,
    const glm::vec3 &startPos)
{
    auto shape = new btBoxShape(btVector3((size.x * 0.5f) * scalef, (size.y * 0.5f) * scalef, (size.z * 0.5f) * scalef));

    return AddObject(shape, mass, startPos, false);
}

PhysicsComponent PhysicsService::AddSphere(
    float mass,
    float radius,
    const glm::vec3 &startPos)
{
    auto shape = new btSphereShape(radius);

    return AddObject(shape, mass, startPos, false);
}

class FindGround : public btCollisionWorld::ContactResultCallback
{
public:
    btScalar addSingleResult(
        btManifoldPoint &cp,
        const btCollisionObjectWrapper *colObj0, int partId0, int index0,
        const btCollisionObjectWrapper *colObj1, int partId1, int index1);

    btRigidBody *mMe = nullptr;
    // Assign some values, in some way
    float mShapeHalfHeight = 0.0f;
    float mRadiusThreshold = 4.0f;
    float mMaxCosGround = 0.0f;
    bool mHaveGround = false;
    btVector3 mGroundPoint;
};

btScalar FindGround::addSingleResult(
    btManifoldPoint &cp,
    const btCollisionObjectWrapper *colObj0, int partId0, int index0,
    const btCollisionObjectWrapper *colObj1, int partId1, int index1)
{
    (void)partId0;
    (void)index0;
    (void)colObj1;
    (void)partId1;
    (void)index1;

    if (colObj0->m_collisionObject == mMe && !mHaveGround)
    {
        const btTransform &transform = mMe->getWorldTransform();
        // Orthonormal basis (just rotations) => can just transpose to invert
        btMatrix3x3 invBasis = transform.getBasis().transpose();
        btVector3 localPoint = invBasis * (cp.m_positionWorldOnB - transform.getOrigin());
        localPoint[2] += mShapeHalfHeight;
        float r = localPoint.length();
        float cosTheta = localPoint[2] / r;

        if (fabs(r - (16 * scalef)) <= mRadiusThreshold && cosTheta < mMaxCosGround)
        {
            mHaveGround = true;
            mGroundPoint = cp.m_positionWorldOnB;
        }
    }

    return 0;
}

void PhysicsService::Step(
    std::chrono::microseconds diff)
{
    // spdlog::info("Step({})", diff.count());
    static long long timeInUs = 0;

    timeInUs += diff.count();

    while (timeInUs > (200000.0 / 60.0))
    {
        // spdlog::info("  timeInMs = {}", timeInUs);
        mDynamicsWorld->stepSimulation(btScalar(1.0f / 60.0f), 1, btScalar(1.0f / 120.0f));

        timeInUs -= (200000.0 / 60.0);
    }
    //    mDynamicsWorld->stepSimulation(static_cast<double>(diff.count()) / 100.0, 1, btScalar(1.0f / 120.0f));
}

void PhysicsService::JumpCharacter(
    const PhysicsComponent &component,
    const glm::vec3 &direction)
{
    FindGround callback;
    callback.mMe = _rigidBodies[component.bodyIndex];
    mDynamicsWorld->contactTest(_rigidBodies[component.bodyIndex], callback);

    if (callback.mHaveGround)
    {
        ApplyForce(component, direction * 18000.0f);
    }
}

void PhysicsService::MoveCharacter(
    const PhysicsComponent &component,
    const glm::vec3 &direction,
    float speed)
{
    btVector3 move(direction.x, direction.y, 0.0f);

    FindGround callback;
    callback.mMe = _rigidBodies[component.bodyIndex];
    mDynamicsWorld->contactTest(_rigidBodies[component.bodyIndex], callback);
    bool onGround = callback.mHaveGround;

    btVector3 linearVelocity = _rigidBodies[component.bodyIndex]->getLinearVelocity();
    auto downVelocity = linearVelocity.z();
    if (move.fuzzyZero() && onGround)
    {
        linearVelocity *= 0.8f;
    }
    else
    {
        linearVelocity = linearVelocity + (move * speed);

        if (linearVelocity.length() > speed)
        {
            linearVelocity *= (speed / linearVelocity.length());
        }
    }

    linearVelocity.setZ(downVelocity);
    _rigidBodies[component.bodyIndex]->setLinearVelocity(linearVelocity);
}

void PhysicsService::ApplyForce(
    const PhysicsComponent &component,
    const glm::vec3 &force,
    const glm::vec3 &relativePosition)
{
    _rigidBodies[component.bodyIndex]->applyForce(
        btVector3(force.x, force.y, force.z),
        btVector3(relativePosition.x * scalef, relativePosition.y * scalef, relativePosition.z * scalef));
}

glm::mat4 PhysicsService::GetMatrix(
    const PhysicsComponent &component)
{
    glm::mat4 mat;

    _rigidBodies[component.bodyIndex]->getWorldTransform().getOpenGLMatrix(glm::value_ptr(mat));

    mat[3].x /= scalef;
    mat[3].y /= scalef;
    mat[3].z /= scalef;

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
