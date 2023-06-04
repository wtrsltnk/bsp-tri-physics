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

    bool SetupBsp(
        valve::hl1::BspAsset *bspAsset,
        glm::vec3 &origin);

    bool SetupEntities(
        valve::hl1::BspAsset *bspAsset,
        glm::vec3 &origin);

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

    void HandleBspInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    void HandleMdlInput(
        std::chrono::microseconds time,
        const struct InputState &inputState);

private:
    std::unique_ptr<IRenderer> _renderer;
    std::unique_ptr<PhysicsService> _physics;
    AssetManager _assets;
    std::string _map;
    valve::hl1::BspAsset *bspAsset = nullptr;
    valve::hl1::MdlAsset *mdlAsset = nullptr;
    valve::hl1::SprAsset *sprAsset = nullptr;
    glm::mat4 _projectionMatrix = glm::mat4(1.0f);
    BufferType _vertexBuffer;
    int _firstSkyVertex = 0;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    ShaderType _defaultShader;
    GLuint _emptyWhiteTexture = 0;
    std::vector<GLuint> _textureIndices;
    std::vector<GLuint> _lightmapIndices;
    std::vector<valve::tFace> _faces;
    Camera _cam;
    bool _physicsCameraMode = false;
    entt::registry _registry;
    PhysicsComponent _character;

    int _firstCubeVertex = 0;
    static float vertices[216];

    bool SetupRenderComponent(
        const entt::entity &entity,
        RenderModes mode);

    bool SetupOriginComponent(
        const entt::entity &entity,
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
