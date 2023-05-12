#ifndef GENMAPAPP_H
#define GENMAPAPP_H

#include "camera.h"
#include "entitycomponents.h"
#include "physicsservice.hpp"
#include "valve/bsp/hl1bspasset.h"
#include "valve/hl1filesystem.h"

#include <chrono>
#include <entt/entt.hpp>
#include <glbuffer.h>
#include <glm/glm.hpp>
#include <glshader.h>
#include <irenderer.hpp>
#include <map>
#include <string>
#include <vector>

class FaceType
{
public:
    GLuint firstVertex;
    GLuint vertexCount;
    GLuint textureIndex;
    GLuint lightmapIndex;
    int flags;
};

class AssetManager
{
public:
    FileSystem _fs;

    valve::Asset *LoadAsset(
        const std::string &name);

    template <typename T>
    T *LoadAsset(
        const std::string &name)
    {
        return reinterpret_cast<T *>(LoadAsset(name));
    }

    valve::Asset *GetAsset(
        long id);

    template <typename T>
    T *GetAsset(
        long id)
    {
        return reinterpret_cast<T *>(GetAsset(id));
    }

private:
    std::map<std::string, std::unique_ptr<valve::Asset>> _loadedAssets;
};

class GenMapApp
{
public:
    enum SkyTextures
    {
        Back = 0,
        Down = 1,
        Front = 2,
        Left = 3,
        Right = 4,
        up = 5,
        Count,
    };

    void SetFilename(
        const char *root,
        const char *map);

    bool Startup();

    void Resize(
        int width,
        int height);

    void Destroy();

    bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState);

protected:
    void SetupSky();

    void SetupBsp();

    glm::vec3 SetupEntities();

    void RenderSky();

    void RenderBsp();

    void RenderModelsByRenderMode(
        RenderModes mode,
        ShaderType &shader,
        const glm::mat4 &matrix);

private:
    std::unique_ptr<IRenderer> _renderer;
    AssetManager _assets;
    std::string _map;
    valve::hl1::BspAsset *_bspAsset = nullptr;
    glm::mat4 _projectionMatrix = glm::mat4(1.0f);
    ShaderType _trailShader;
    ShaderType _skyShader;
    BufferType _skyVertexBuffer;
    BufferType _vertexArray;
    BufferType _studioVertexArray;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    ShaderType _normalBlendingShader;
    ShaderType _solidBlendingShader;
    BufferType _vertexBuffer;
    std::vector<GLuint> _textureIndices;
    std::vector<GLuint> _lightmapIndices;
    std::vector<FaceType> _faces;
    std::map<GLuint, FaceType> _facesByLightmapAtlas;
    Camera _cam;
    entt::registry _registry;
    PhysicsService *_physics = nullptr;
    PhysicsComponent _character;

    static float vertices[216];
};

#endif // GENMAPAPP_H
