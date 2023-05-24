#ifndef GENMAPAPP_H
#define GENMAPAPP_H

#include "assetmanager.h"
#include "camera.h"
#include "entitycomponents.h"
#include "physicsservice.hpp"
#include "valve/bsp/hl1bspasset.h"
#include "valve/mdl/hl1mdlinstance.h"

#include <application.h>
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

class GenMapApp : public IApplication
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

    virtual void SetFilename(
        const char *root,
        const char *map);

    virtual bool Startup();

    virtual void Resize(
        int width,
        int height);

    virtual void Destroy();

    virtual bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState);

protected:
    void SetupSky(
        valve::hl1::BspAsset *bspAsset);

    glm::vec3 SetupBsp(
        valve::hl1::BspAsset *bspAsset);

    glm::vec3 SetupEntities(
        valve::hl1::BspAsset *bspAsset);

    bool RenderAsset(
        std::chrono::microseconds time);

    void RenderSky();

    void RenderBsp(
        valve::hl1::BspAsset *bspAsset,
        std::chrono::microseconds time);

    void RenderModelsByRenderMode(
        valve::hl1::BspAsset *bspAsset,
        RenderModes mode,
        ShaderType &shader,
        const glm::mat4 &matrix);

    void RenderStudioModelsByRenderMode(
        RenderModes mode,
        ShaderType &shader,
        const glm::mat4 &matrix,
        std::chrono::microseconds time);

private:
    std::unique_ptr<IRenderer> _renderer;
    AssetManager _assets;
    std::string _map;
    valve::Asset *_rootAsset = nullptr;
    valve::hl1::MdlInstance _mdlInstance;
    glm::mat4 _projectionMatrix = glm::mat4(1.0f);
    ShaderType _trailShader;
    ShaderType _skyShader;
    BufferType _skyVertexBuffer;
    BufferType _vertexArray;
    BufferType _studioVertexArray;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    ShaderType _normalBlendingShader;
    ShaderType _studioNormalBlendingShader;
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

    bool SetupRenderComponent(
        const entt::entity &entity,
        RenderModes mode,
        ShaderType &shader,
        const glm::mat4 &matrix);
};

#endif // GENMAPAPP_H
