#ifndef ENTITYCOMPONENTS_H
#define ENTITYCOMPONENTS_H

#include <glm/vec3.hpp>
#include <string>

struct BallComponent
{
    int code;
};

struct OriginComponent
{
    glm::vec3 Origin;
    glm::vec3 Angles;
};

struct ModelComponent
{
    long AssetId = 0;
    int Model;
};

struct SpriteComponent
{
    long AssetId = 0;
    float Scale = 1.0f;
    int FirstVertexInBuffer;
    int VertexCount;
    int TextureOffset = 0;
    float Frame = 0;
};

struct StudioComponent
{
    // The bones are different for each instance of mdl so we need to manage
    // the data to the uniformbuffer in the instance, not the asset
    unsigned int _bonesBuffer;
    long AssetId = 0;
    float Scale = 1.0f;
    int FirstVertexInBuffer;
    int VertexCount;
    int TextureOffset = 0;
    int LightmapOffset = 0;
    int Sequence = 0;                   // sequence index
    float Frame = 0;                    // frame
    bool Repeat = true;                 // repeat after end of sequence
    int Skinnum = 0;                    // skin group selection
    short Controller[4] = {0, 0, 0, 0}; // bone controllers
    short Blending[2] = {0, 0};         // animation blending
    short Mouth = 0;                    // mouth position
};

enum RenderModes
{
    NormalBlending = 0,
    ColorBlending = 1,
    TextureBlending = 2,
    GlowBlending = 3,
    SolidBlending = 4,
    AdditiveBlending = 5,
};

struct RenderComponent
{
    short Amount;
    short Color[3];
    RenderModes Mode;
};

#endif // ENTITYCOMPONENTS_H
