#ifndef GENMAPAPP_H
#define GENMAPAPP_H

#include "assetmanager.h"
#include "camera.h"
#include "entitycomponents.h"
#include "physicsservice.hpp"
#include "valve/bsp/hl1bspasset.h"
#include "valve/mdl/hl1mdlasset.h"
#include "valve/spr/hl1sprasset.h"

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

    void RenderByRenderMode(
        valve::hl1::BspAsset *bspAsset,
        RenderModes mode,
        ShaderType shaders[3],
        std::chrono::microseconds time);

    void RenderModelsByRenderMode(
        valve::hl1::BspAsset *bspAsset,
        RenderModes mode,
        ShaderType &shader);

    void RenderStudioModelsByRenderMode(
        RenderModes mode,
        ShaderType &shader,
        std::chrono::microseconds time);

    void RenderSpritesByRenderMode(
        RenderModes mode,
        ShaderType &shader,
        std::chrono::microseconds time);

    void HandleBspInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    void HandleMdlInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

private:
    std::unique_ptr<IRenderer> _renderer;
    AssetManager _assets;
    std::string _map;
    valve::Asset *_rootAsset = nullptr;
    glm::mat4 _projectionMatrix = glm::mat4(1.0f);
    ShaderType _trailShader;
    ShaderType _skyShader;
    BufferType _skyVertexBuffer;
    BufferType _vertexArray;
    BufferType _studioVertexArray;
    BufferType _spriteVertexArray;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    ShaderType _normalBlendingShader;
    ShaderType _studioNormalBlendingShader;
    ShaderType _spriteNormalBlendingShader;
    ShaderType _solidBlendingShader;
    BufferType _vertexBuffer;
    std::vector<GLuint> _textureIndices;
    std::vector<GLuint> _lightmapIndices;
    std::vector<valve::tFace> _faces;
    std::map<GLuint, valve::tFace> _facesByLightmapAtlas;
    Camera _cam;
    bool _physicsCameraMode = false;
    entt::registry _registry;
    PhysicsService *_physics = nullptr;
    PhysicsComponent _character;

    static float vertices[216];

    bool SetupRenderComponent(
        const entt::entity &entity,
        RenderModes mode,
        ShaderType &shader);

    bool SetupOriginComponent(
        const entt::entity &entity,
        ShaderType &shader,
        float scale = 1.0f);

    StudioComponent BuildStudioComponent(
        valve::hl1::MdlAsset *mdlAsset,
        float scale = 1.0f);

    SpriteComponent BuildSpriteComponent(
        valve::hl1::SprAsset *sprAsset,
        float scale = 1.0f);

    void GrabTriangles(
        valve::hl1::BspAsset *bspAsset,
        int moddelIndex,
        std::vector<glm::vec3> &triangles);
};

#endif // GENMAPAPP_H
