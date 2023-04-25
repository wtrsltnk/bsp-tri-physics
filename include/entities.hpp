#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include <entt/entt.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct MeshComponent
{
    GLuint vao = 0;
    size_t asset = 0;
    int node = -1;
};

struct PhysicsComponent
{
    size_t bodyIndex = 0;
};

struct CarComponent
{
    size_t carIndex = 0;
    size_t wheelCount = 4;
    float engineForce = 0.0f;
};

struct WheelComponent
{
    size_t carIndex = 0;
    size_t wheelIndex = 0;
};

enum TrackTileTypes
{
    CornerNE,
    CornerSE,
    CornerSW,
    CornerNW,
    Horizontal,
    Vertical,
};

struct TrackTileComponent
{
    TrackTileTypes type;
};

struct TransformationComponent
{
    glm::mat4 matrix;
};

#endif // ENTITIES_HPP
