#ifndef ENGINE_H
#define ENGINE_H

#include "camera.h"
#include "entitycomponents.h"

#include <entt/entt.hpp>
#include <glbuffer.h>
#include <iassetmanager.hpp>
#include <inputstate.h>
#include <iphysicsservice.hpp>
#include <irenderer.hpp>
#include <valve/bsp/hl1bspasset.h>
#include <valve/mdl/hl1mdlasset.h>
#include <valve/spr/hl1sprasset.h>

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

class Engine
{
public:
    Engine(
        IRenderer *renderer,
        IPhysicsService *physicsService,
        IAssetManager *assetManager);

    virtual ~Engine();

    void SetProjectionMatrix(
        const glm::mat4 &projectionMatrix);

    bool Load(
        const std::string &asset);

    void Update(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    bool Render(
        std::chrono::microseconds time);

private:
    IRenderer *_renderer;
    IPhysicsService *_physicsService;
    IAssetManager *_assetManager;
    entt::registry _registry;
    Camera _cam;
    glm::mat4 _projectionMatrix;

    valve::hl1::BspAsset *bspAsset = nullptr;
    valve::hl1::MdlAsset *mdlAsset = nullptr;
    valve::hl1::SprAsset *sprAsset = nullptr;

    // Render data
    BufferType _vertexBuffer;
    std::unique_ptr<IShader> _defaultShader;
    std::vector<valve::tFace> _faces;
    std::vector<GLuint> _textureIndices;
    std::vector<GLuint> _lightmapIndices;
    int _firstSkyVertex = 0;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    GLuint _emptyWhiteTexture = 0;

    // Game logic
    PhysicsComponent _character;

    // For Load()
    bool SetupBsp(
        valve::hl1::BspAsset *bspAsset);

    bool SetupEntities(
        valve::hl1::BspAsset *bspAsset);

    StudioComponent BuildStudioComponent(
        valve::hl1::MdlAsset *mdlAsset,
        float scale = 1.0f);

    SpriteComponent BuildSpriteComponent(
        valve::hl1::SprAsset *sprAsset,
        float scale = 1.0f);

    OriginComponent BuildOriginComponent(
        valve::hl1::tBSPEntity &bspEntity);

    void SetupSky(
        valve::hl1::BspAsset *bspAsset);

    void GrabTriangles(
        valve::hl1::BspAsset *bspAsset,
        int moddelIndex,
        std::vector<glm::vec3> &triangles);

    // For Update()
    void HandleBspInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    void HandleMdlInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    // For Render()
    bool RenderAsset(
        std::chrono::microseconds time);

    void RenderSky();

    void RenderBsp(
        valve::hl1::BspAsset *bspAsset,
        std::chrono::microseconds time);

    void RenderByRenderMode(
        valve::hl1::BspAsset *bspAsset,
        RenderModes mode,
        std::chrono::microseconds time);

    void RenderModelsByRenderMode(
        valve::hl1::BspAsset *bspAsset,
        RenderModes mode);

    void RenderStudioModelsByRenderMode(
        RenderModes mode,
        std::chrono::microseconds time);

    void RenderSpritesByRenderMode(
        RenderModes mode,
        std::chrono::microseconds time);

    bool SetupRenderComponent(
        const entt::entity &entity,
        RenderModes mode);

    bool SetupOriginComponent(
        const entt::entity &entity,
        float scale = 1.0f);
};

#endif // ENGINE_H
