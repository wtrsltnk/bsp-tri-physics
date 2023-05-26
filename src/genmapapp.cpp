#include "genmapapp.h"

#include "renderers/opengl/openglrenderer.hpp"
#include "valve/mdl/hl1mdlinstance.h"
#include <application.h>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stb_image.h>

void EnableOpenGlDebug();

extern const char *trailVertexShader;
extern const char *trailFragmentShader;

template <class T>
inline std::istream &operator>>(
    std::istream &str, T &v)
{
    unsigned int tmp = 0;

    if (str >> tmp)
    {
        v = static_cast<T>(tmp);
    }

    return str;
}

bool GenMapApp::Startup()
{
    _renderer = std::make_unique<OpenGlRenderer>();
    _physics = new PhysicsService();

    spdlog::debug("Startup()");

    EnableOpenGlDebug();

    glClearColor(0.0f, 0.45f, 0.7f, 1.0f);

    spdlog::info("{} @ {}", _assets._fs.Mod(), _assets._fs.Root().generic_string());

    _rootAsset = _assets.LoadAsset(_map);

    if (_rootAsset == nullptr)
    {
        spdlog::error("Failed to load {}", _map);

        return false;
    }

    auto sprAsset = dynamic_cast<valve::hl1::SprAsset *>(_rootAsset);

    if (sprAsset != nullptr)
    {
        const auto entity = _registry.create();

        OriginComponent originComponent = {
            .Origin = glm::vec3(0.0f),
            .Angles = glm::vec3(0.0f),
        };

        _registry.assign<OriginComponent>(entity, originComponent);

        RenderComponent rc = {0, {255, 255, 255}, RenderModes::NormalBlending};

        _registry.assign<RenderComponent>(entity, rc);

        _registry.assign<SpriteComponent>(entity, BuildSpriteComponent(sprAsset));

        _spriteNormalBlendingShader.compileSprShader();
        _spriteVertexArray.upload(_spriteNormalBlendingShader, false);

        _cam.SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));

        return true;
    }

    auto mdlAsset = dynamic_cast<valve::hl1::MdlAsset *>(_rootAsset);

    if (mdlAsset != nullptr)
    {
        auto center = mdlAsset->_header->min + ((mdlAsset->_header->max - mdlAsset->_header->min) * 0.5f);

        const auto entity = _registry.create();

        OriginComponent originComponent = {
            .Origin = glm::vec3(0.0f),
            .Angles = glm::vec3(0.0f),
        };

        _registry.assign<OriginComponent>(entity, originComponent);

        RenderComponent rc = {0, {255, 255, 255}, RenderModes::NormalBlending};

        _registry.assign<RenderComponent>(entity, rc);

        _registry.assign<StudioComponent>(entity, BuildStudioComponent(mdlAsset));

        _studioNormalBlendingShader.compileMdlShader();
        _studioVertexArray.upload(_studioNormalBlendingShader, true);

        auto offset = glm::length(center);
        if (offset == 0.0f)
        {
            offset = 50.0f;
        }

        _cam.SetPosition(center + glm::vec3(0.0f, offset, 0.0f));

        return true;
    }

    auto bspAsset = dynamic_cast<valve::hl1::BspAsset *>(_rootAsset);

    if (bspAsset != nullptr)
    {
        auto origin = SetupBsp(bspAsset);

        _trailShader.compile(trailVertexShader, trailFragmentShader);

        for (int v = 0; v < 36; v++)
        {
            _vertexArray
                .vertex_and_col(&vertices[v * 6], glm::vec3(10.0f));
        }

        _vertexArray.upload(_trailShader);

        std::vector<glm::vec3> triangles;

        auto &rootModel = bspAsset->_models[0];

        for (int f = rootModel.firstFace; f < rootModel.firstFace + rootModel.faceCount; f++)
        {
            auto &face = bspAsset->_faces[f];

            // if (face.flags != 0)
            // {
            //     continue;
            // }

            auto &vertex1 = bspAsset->_vertices[face.firstVertex];
            auto &vertex2 = bspAsset->_vertices[face.firstVertex + 1];

            for (int v = face.firstVertex + 2; v < face.firstVertex + face.vertexCount; v++)
            {
                triangles.push_back(vertex1.position);
                triangles.push_back(vertex2.position);
                triangles.push_back(bspAsset->_vertices[v].position);

                vertex2 = bspAsset->_vertices[v];
            }
        }

        _physics->AddStatic(triangles);

        _character = _physics->AddCharacter(15, 16, 45, origin);

        return true;
    }

    return false;
}

SpriteComponent GenMapApp::BuildSpriteComponent(
    valve::hl1::SprAsset *sprAsset)
{
    SpriteComponent sc = {
        .AssetId = sprAsset->Id(),
        .FirstVertexInBuffer = static_cast<int>(_spriteVertexArray.vertexCount()),
        .VertexCount = static_cast<int>(sprAsset->_vertices.size()),
    };

    sc.TextureOffset = static_cast<int>(_textureIndices.size());

    glActiveTexture(GL_TEXTURE0);
    for (size_t i = 0; i < sprAsset->_textures.size(); i++)
    {
        auto &tex = sprAsset->_textures[i];
        _textureIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    for (auto &vert : sprAsset->_vertices)
    {
        _spriteVertexArray
            .uvs(vert.texcoords)
            .vertex(vert.position);
    }

    return sc;
}

StudioComponent GenMapApp::BuildStudioComponent(
    valve::hl1::MdlAsset *mdlAsset)
{
    StudioComponent sc = {
        .AssetId = mdlAsset->Id(),
        .FirstVertexInBuffer = static_cast<int>(_studioVertexArray.vertexCount()),
        .VertexCount = static_cast<int>(mdlAsset->_vertices.size()),
    };

    sc.LightmapOffset = static_cast<int>(_lightmapIndices.size());
    glActiveTexture(GL_TEXTURE1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (size_t i = 0; i < mdlAsset->_lightmaps.size(); i++)
    {
        auto &tex = mdlAsset->_lightmaps[i];
        _lightmapIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    sc.TextureOffset = static_cast<int>(_textureIndices.size());
    glActiveTexture(GL_TEXTURE0);
    for (size_t i = 0; i < mdlAsset->_textures.size(); i++)
    {
        auto &tex = mdlAsset->_textures[i];
        _textureIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    for (auto &vert : mdlAsset->_vertices)
    {
        _studioVertexArray
            .bone(vert.bone)
            .uvs(vert.texcoords)
            .vertex(vert.position);
    }

    return sc;
}

bool showPhysicsDebug = false;

void GenMapApp::HandleBspInput(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    (void)time;

    const float speed = 8.0f;

    if (IsKeyboardButtonPushed(inputState, KeyboardButtons::KeySpace))
    {
        _physics->JumpCharacter(_character, _cam.Up());
    }

    if (IsKeyboardButtonPushed(inputState, KeyboardButtons::KeyF8))
    {
        showPhysicsDebug = !showPhysicsDebug;
    }

    if (inputState.KeyboardButtonStates[KeyboardButtons::KeyLeft] || inputState.KeyboardButtonStates[KeyboardButtons::KeyA])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            _physics->MoveCharacter(_character, _cam.Forward() + _cam.Left(), speed);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            _physics->MoveCharacter(_character, _cam.Back() + _cam.Left(), speed);
        }
        else
        {
            _physics->MoveCharacter(_character, _cam.Left(), speed);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyRight] || inputState.KeyboardButtonStates[KeyboardButtons::KeyD])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            _physics->MoveCharacter(_character, _cam.Forward() + _cam.Right(), speed);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            _physics->MoveCharacter(_character, _cam.Back() + _cam.Right(), speed);
        }
        else
        {
            _physics->MoveCharacter(_character, _cam.Right(), speed);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
    {
        _physics->MoveCharacter(_character, _cam.Forward(), speed);
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
    {
        _physics->MoveCharacter(_character, _cam.Back(), speed);
    }
    else
    {
        _physics->MoveCharacter(_character, glm::vec3(0.0f), speed);
    }

    auto m = _physics->GetMatrix(_character);
    _cam.SetPosition(glm::vec3(m[3]) + glm::vec3(0.0f, 0.0f, 32.0f));
}

void GenMapApp::HandleMdlInput(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    const float speed = 40.0f;
    auto dt = float(double(time.count()) / 1000000.0);

    auto pos = _cam.Position();

    if (inputState.KeyboardButtonStates[KeyboardButtons::KeyLeft] || inputState.KeyboardButtonStates[KeyboardButtons::KeyA])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            pos = pos + ((_cam.Forward() + _cam.Left()) * speed * dt);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            pos = pos + ((_cam.Back() + _cam.Left()) * speed * dt);
        }
        else
        {
            pos = pos + ((_cam.Left()) * speed * dt);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyRight] || inputState.KeyboardButtonStates[KeyboardButtons::KeyD])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            pos = pos + ((_cam.Forward() + _cam.Right()) * speed * dt);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            pos = pos + ((_cam.Back() + _cam.Right()) * speed * dt);
        }
        else
        {
            pos = pos + ((_cam.Right()) * speed * dt);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
    {
        pos = pos + ((_cam.Forward()) * speed * dt);
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
    {
        pos = pos + ((_cam.Back()) * speed * dt);
    }

    _cam.SetPosition(pos);
}

bool GenMapApp::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    _physics->Step(time);

    if (IsMouseButtonPushed(inputState, MouseButtons::LeftButton))
    {
        const auto entity = _registry.create();

        auto comp = _physics->AddCube(1.0f, glm::vec3(30.0f), _cam.Position() + (_cam.Forward() * 40.0f));

        _physics->ApplyForce(comp, glm::normalize(_cam.Forward()) * 7000.0f);

        _registry.assign<PhysicsComponent>(entity, comp);
        _registry.assign<BallComponent>(entity, 0);
    }

    if (_character.bodyIndex > 0)
    {
        HandleBspInput(time, inputState);
    }
    else
    {
        HandleMdlInput(time, inputState);
    }

    _cam.ProcessMouseMovement(
        float(inputState.MousePointerPosition[0]),
        float(inputState.MousePointerPosition[1]),
        true);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderAsset(time);

    glDisable(GL_CULL_FACE);

    _vertexArray.bind();

    auto view = _registry.view<PhysicsComponent, BallComponent>();

    for (auto &entity : view)
    {
        auto physicsComponent = _registry.get<PhysicsComponent>(entity);

        auto m = glm::scale(_physics->GetMatrix(physicsComponent), glm::vec3(3.0f));

        _trailShader.setupMatrices(_projectionMatrix * _cam.GetViewMatrix() * m);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glEnable(GL_CULL_FACE);

    if (showPhysicsDebug)
    {
        VertexArray vertexAndColorBuffer;

        _physics->RenderDebug(vertexAndColorBuffer);

        vertexAndColorBuffer.upload();

        _trailShader.setupMatrices(_projectionMatrix * _cam.GetViewMatrix() * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 0.08f, 1.0f / 0.08f, 1.0f / 0.08f)));
        glDisable(GL_DEPTH_TEST);
        vertexAndColorBuffer.render(VertexArrayRenderModes::Lines);
        glEnable(GL_DEPTH_TEST);
    }

    return true; // to keep running
}

bool GenMapApp::RenderAsset(
    std::chrono::microseconds time)
{
    auto sprAsset = dynamic_cast<valve::hl1::SprAsset *>(_rootAsset);

    if (sprAsset != nullptr)
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        auto m = _projectionMatrix * _cam.GetViewMatrix();

        glDisable(GL_BLEND);

        RenderSpritesByRenderMode(RenderModes::NormalBlending, _spriteNormalBlendingShader, m, time);

        return true;
    }

    auto mdlAsset = dynamic_cast<valve::hl1::MdlAsset *>(_rootAsset);

    if (mdlAsset != nullptr)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        auto m = _projectionMatrix * _cam.GetViewMatrix();

        glDisable(GL_BLEND);

        RenderStudioModelsByRenderMode(RenderModes::NormalBlending, _studioNormalBlendingShader, m, time);

        return true;
    }

    auto bspAsset = dynamic_cast<valve::hl1::BspAsset *>(_rootAsset);

    if (bspAsset != nullptr)
    {
        RenderSky();
        RenderBsp(bspAsset, time);

        return true;
    }

    return false;
}

void GenMapApp::SetFilename(
    const char *root,
    const char *map)
{
    _assets._fs.FindRootFromFilePath(root);
    _map = map;
}

glm::vec3 GenMapApp::SetupBsp(
    valve::hl1::BspAsset *bspAsset)
{
    _lightmapIndices = std::vector<GLuint>();
    glActiveTexture(GL_TEXTURE1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (size_t i = 0; i < bspAsset->_lightMaps.size(); i++)
    {
        auto &tex = bspAsset->_lightMaps[i];
        _lightmapIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    _textureIndices = std::vector<GLuint>();
    glActiveTexture(GL_TEXTURE0);
    for (size_t i = 0; i < bspAsset->_textures.size(); i++)
    {
        auto &tex = bspAsset->_textures[i];
        _textureIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    for (size_t f = 0; f < bspAsset->_faces.size(); f++)
    {
        auto &face = bspAsset->_faces[f];

        valve::tFace ft;

        ft.flags = face.flags;
        ft.firstVertex = 0;
        ft.vertexCount = 0;
        ft.lightmap = 0;
        ft.texture = 0;

        // if (face.flags == 0)
        {
            ft.firstVertex = _vertexBuffer.vertexCount();
            ft.vertexCount = face.vertexCount;
            ft.lightmap = static_cast<unsigned int>(f);
            ft.texture = face.texture;

            for (int v = face.firstVertex; v < face.firstVertex + face.vertexCount; v++)
            {
                auto &vertex = bspAsset->_vertices[v];

                _vertexBuffer
                    .uvs(glm::vec4(vertex.texcoords[1].x, vertex.texcoords[1].y, vertex.texcoords[0].x, vertex.texcoords[0].y))
                    .bone(vertex.bone)
                    .vertex(glm::vec3(vertex.position));
            }
        }

        _faces.push_back(ft);
    }

    _solidBlendingShader.compileBspShader();

    _normalBlendingShader.compileDefaultShader();
    _vertexBuffer.upload(_normalBlendingShader);

    auto origin = SetupEntities(bspAsset);

    _studioNormalBlendingShader.compileMdlShader();
    _studioVertexArray.upload(_studioNormalBlendingShader, true);

    _spriteNormalBlendingShader.compileSprShader();
    _spriteVertexArray.upload(_spriteNormalBlendingShader, false);

    return origin;
}

glm::vec3 GenMapApp::SetupEntities(
    valve::hl1::BspAsset *bspAsset)
{
    for (auto &bspEntity : bspAsset->_entities)
    {
        const auto entity = _registry.create();

        if (bspEntity.classname == "worldspawn")
        {
            _registry.assign<ModelComponent>(entity, 0);

            SetupSky(bspAsset);

            _skyShader.compileSkyShader();
            _skyVertexBuffer.upload(_skyShader);
        }

        if (bspEntity.classname == "info_player_deathmatch")
        {
            std::istringstream iss(bspEntity.keyvalues["origin"]);
            int x, y, z;

            iss >> x >> y >> z;

            _cam.SetPosition(glm::vec3(x, y, z));
        }

        if (bspEntity.keyvalues.count("model") != 0 && bspEntity.classname.rfind("hostage_entity", 0) != 0)
        {
            ModelComponent mc = {
                .AssetId = bspAsset->Id(),
                .Model = 0,
            };

            std::istringstream iss(bspEntity.keyvalues["model"]);
            iss.get(); // get the astrix
            iss >> (mc.Model);

            if (mc.Model != 0)
            {
                _registry.assign<ModelComponent>(entity, mc);
            }
            else
            {
                auto asset = _assets.LoadAsset(bspEntity.keyvalues["model"]);

                auto sprAsset = dynamic_cast<valve::hl1::SprAsset *>(asset);
                auto mdlAsset = dynamic_cast<valve::hl1::MdlAsset *>(asset);

                if (sprAsset != nullptr)
                {
                    _registry.assign<SpriteComponent>(entity, BuildSpriteComponent(sprAsset));
                }
                else if (mdlAsset != nullptr)
                {
                    _registry.assign<StudioComponent>(entity, BuildStudioComponent(mdlAsset));
                }
            }
        }

        RenderComponent rc = {0, {255, 255, 255}, RenderModes::NormalBlending};

        auto renderamt = bspEntity.keyvalues.find("renderamt");
        if (renderamt != bspEntity.keyvalues.end())
        {
            std::istringstream(renderamt->second) >> (rc.Amount);
        }

        auto rendercolor = bspEntity.keyvalues.find("rendercolor");
        if (rendercolor != bspEntity.keyvalues.end())
        {
            std::istringstream(rendercolor->second) >> (rc.Color[0]) >> (rc.Color[1]) >> (rc.Color[2]);
        }

        auto rendermode = bspEntity.keyvalues.find("rendermode");
        if (rendermode != bspEntity.keyvalues.end())
        {
            std::istringstream(rendermode->second) >> (rc.Mode);
        }

        _registry.assign<RenderComponent>(entity, rc);

        OriginComponent originComponent = {
            .Origin = glm::vec3(0.0f),
            .Angles = glm::vec3(0.0f),
        };

        auto origin = bspEntity.keyvalues.find("origin");
        if (origin != bspEntity.keyvalues.end())
        {
            glm::vec3 originPosition;

            std::istringstream(origin->second) >> originPosition.x >> originPosition.y >> originPosition.z;

            originComponent.Origin = originPosition;
        }

        auto angles = bspEntity.keyvalues.find("angles");
        auto angle = bspEntity.keyvalues.find("angle");
        if (angles != bspEntity.keyvalues.end())
        {
            glm::vec3 anglesPosition;

            std::istringstream(angles->second) >> anglesPosition.x >> anglesPosition.y >> anglesPosition.z;

            originComponent.Angles = anglesPosition;
        }
        else if (angle != bspEntity.keyvalues.end())
        {
            float anglePosition;

            std::istringstream(angle->second) >> anglePosition;

            originComponent.Angles = glm::vec3(0.0f, anglePosition, 0.0f);
        }

        _registry.assign<OriginComponent>(entity, originComponent);
    }

    auto info_player_start = bspAsset->FindEntityByClassname("info_player_deathmatch");

    glm::vec3 angles(0.0f);
    glm::vec3 origin(0.0f);

    if (info_player_start != nullptr)
    {
        std::istringstream(info_player_start->keyvalues["angles"]) >> (angles.x) >> (angles.y) >> (angles.z);
        //        _cam.RotateX(angles.x);
        //        _cam.RotateY(angles.y);
        //        _cam.RotateZ(angles.z);
        _cam.ProcessMouseMovement(angles.x, angles.y, true);

        std::istringstream(info_player_start->keyvalues["origin"]) >> (origin.x) >> (origin.y) >> (origin.z);
        _cam.SetPosition(origin);
    }

    _registry.sort<RenderComponent>([](const RenderComponent &lhs, const RenderComponent &rhs) {
        return lhs.Mode < rhs.Mode;
    });

    return origin;
}

void GenMapApp::RenderBsp(
    valve::hl1::BspAsset *bspAsset,
    std::chrono::microseconds time)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    auto m = _projectionMatrix * _cam.GetViewMatrix();

    ShaderType shaders[3] = {
        _normalBlendingShader,
        _studioNormalBlendingShader,
        _spriteNormalBlendingShader,
    };

    glDisable(GL_BLEND);
    RenderByRenderMode(bspAsset, RenderModes::NormalBlending, shaders, m, time);
    RenderByRenderMode(bspAsset, RenderModes::ColorBlending, shaders, m, time);
    RenderByRenderMode(bspAsset, RenderModes::GlowBlending, shaders, m, time);
    RenderByRenderMode(bspAsset, RenderModes::AdditiveBlending, shaders, m, time);

    ShaderType solidShaders[3] = {
        _solidBlendingShader,
        _studioNormalBlendingShader,
        _spriteNormalBlendingShader,
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_DST_ALPHA);
    RenderByRenderMode(bspAsset, RenderModes::TextureBlending, solidShaders, m, time);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderByRenderMode(bspAsset, RenderModes::SolidBlending, solidShaders, m, time);
}

void GenMapApp::RenderByRenderMode(
    valve::hl1::BspAsset *bspAsset,
    RenderModes mode,
    ShaderType shaders[3],
    const glm::mat4 &matrix,
    std::chrono::microseconds time)
{
    RenderModelsByRenderMode(bspAsset, mode, shaders[0], matrix);
    RenderStudioModelsByRenderMode(mode, shaders[1], matrix, time);
    RenderSpritesByRenderMode(mode, shaders[2], matrix, time);
}

bool GenMapApp::SetupRenderComponent(
    const entt::entity &entity,
    RenderModes mode,
    ShaderType &shader,
    const glm::mat4 &matrix)
{
    auto renderComponent = _registry.get<RenderComponent>(entity);

    if (renderComponent.Mode != mode)
    {
        return false;
    }

    if (mode == RenderModes::TextureBlending || mode == RenderModes::SolidBlending)
    {
        shader.setupColor(
            glm::vec4(1.0f, 1.0f, 1.0f, float(renderComponent.Amount) / 255.0f));
    }

    auto originComponent = _registry.get<OriginComponent>(entity);

    auto endm = glm::translate(matrix, originComponent.Origin);

    endm = glm::rotate(endm, glm::radians(originComponent.Angles.z), glm::vec3(1.0f, 0.0f, 0.0f));
    endm = glm::rotate(endm, glm::radians(originComponent.Angles.y), glm::vec3(0.0f, 0.0f, 1.0f));
    endm = glm::rotate(endm, glm::radians(originComponent.Angles.x), glm::vec3(0.0f, -1.0f, 0.0f));

    shader.setupMatrices(endm);

    return true;
}

void GenMapApp::RenderModelsByRenderMode(
    valve::hl1::BspAsset *bspAsset,
    RenderModes mode,
    ShaderType &shader,
    const glm::mat4 &matrix)
{
    auto entities = _registry.view<RenderComponent, ModelComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    shader.use();
    shader.setupBrightness(2.0f);

    if (mode == RenderModes::NormalBlending)
    {
        shader.setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    for (auto entity : entities)
    {
        if (!SetupRenderComponent(entity, mode, shader, matrix))
        {
            continue;
        }

        _vertexBuffer.bind();

        auto modelComponent = _registry.get<ModelComponent>(entity);
        auto model = bspAsset->_models[modelComponent.Model];

        for (int i = model.firstFace; i < model.firstFace + model.faceCount; i++)
        {
            if (_faces[i].flags > 0)
            {
                continue;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textureIndices[_faces[i].texture]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _lightmapIndices[_faces[i].lightmap]);

            glDrawArrays(GL_TRIANGLE_FAN, _faces[i].firstVertex, _faces[i].vertexCount);
        }
    }
}

void GenMapApp::RenderSpritesByRenderMode(
    RenderModes mode,
    ShaderType &shader,
    const glm::mat4 &matrix,
    std::chrono::microseconds time)
{
    auto dt = float(double(time.count()) / 1000000.0);

    auto entities = _registry.view<RenderComponent, SpriteComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    glDisable(GL_CULL_FACE);
    shader.use();
    shader.setupBrightness(2.0f);
    shader.setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    for (auto entity : entities)
    {
        auto spriteComponent = _registry.try_get<SpriteComponent>(entity);

        auto asset = _assets.GetAsset<valve::hl1::SprAsset>(spriteComponent->AssetId);

        if (asset == nullptr)
        {
            continue;
        }

        if (!SetupRenderComponent(entity, mode, shader, matrix))
        {
            continue;
        }

        _spriteVertexArray.bind();

        spriteComponent->Frame += (dt * 24.0f);

        if (size_t(spriteComponent->Frame) >= asset->_faces.size())
        {
            spriteComponent->Frame = 0;
        }

        auto &face = asset->_faces[size_t(spriteComponent->Frame)];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureIndices[spriteComponent->TextureOffset + face.texture]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDrawArrays(GL_TRIANGLE_FAN, spriteComponent->FirstVertexInBuffer + face.firstVertex, face.vertexCount);

        _spriteVertexArray.unbind();
    }
    glEnable(GL_CULL_FACE);
}

void GenMapApp::RenderStudioModelsByRenderMode(
    RenderModes mode,
    ShaderType &shader,
    const glm::mat4 &matrix,
    std::chrono::microseconds time)
{
    auto entities = _registry.view<RenderComponent, StudioComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    shader.use();
    shader.setupBrightness(1.0f);

    if (mode == RenderModes::NormalBlending)
    {
        shader.setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    valve::hl1::MdlInstance _mdlInstance;
    for (auto entity : entities)
    {
        auto studioComponent = _registry.try_get<StudioComponent>(entity);

        auto asset = _assets.GetAsset<valve::hl1::MdlAsset>(studioComponent->AssetId);

        if (asset == nullptr)
        {
            continue;
        }

        _mdlInstance.Asset = asset;
        _mdlInstance.SetMouth(studioComponent->Mouth);
        _mdlInstance.SetSequence(studioComponent->Sequence, studioComponent->Repeat);

        for (int i = 0; i < 4; i++)
        {
            _mdlInstance.SetBlending(i, studioComponent->Blending[i]);
            _mdlInstance.SetController(i, studioComponent->Controller[i]);
        }

        studioComponent->Frame = _mdlInstance.Update(studioComponent->Frame, time);

        shader.BindBones(_mdlInstance._bonetransform, _mdlInstance.Asset->_boneData.size());

        if (!SetupRenderComponent(entity, mode, shader, matrix))
        {
            continue;
        }

        _studioVertexArray.bind();

        std::set<size_t> indices;
        for (size_t bi = 0; bi < asset->_bodyparts.size(); bi++)
        {
            auto &b = asset->_bodyparts[bi];
            for (size_t mi = 0; mi < b.models.size(); mi++)
            {
                auto &m = b.models[mi];
                for (size_t e = 0; e < m.faces.size(); e++)
                {
                    indices.insert(m.firstFace + e);
                }
            }
        }

        for (auto &faceIndex : indices)
        {
            auto &face = asset->_faces[faceIndex];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textureIndices[studioComponent->TextureOffset + face.texture]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _lightmapIndices[studioComponent->LightmapOffset + face.lightmap]);

            glDrawArrays(GL_TRIANGLES, studioComponent->FirstVertexInBuffer + face.firstVertex, face.vertexCount);
        }

        _studioVertexArray.unbind();

        shader.UnbindBones();
    }
}

void GenMapApp::Resize(
    int width,
    int height)
{
    glViewport(0, 0, width, height);

    _projectionMatrix = glm::perspective(glm::radians(90.0f), float(width) / float(height), 0.1f, 4096.0f);
}

void GenMapApp::Destroy()
{
}

void GenMapApp::SetupSky(
    valve::hl1::BspAsset *bspAsset)
{
    for (int i = 0; i < 6; i++)
    {
        auto &tex = bspAsset->_skytextures[i];
        _skyTextureIndices[i] = _renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data());
    }

    // here we make up for the half of pixel to get the sky textures really stitched together because clamping is not enough
    const float uv_1 = 255.0f / 256.0f;
    const float uv_0 = 1.0f - uv_1;
    const float size = 1.0f;

    // if (renderFlag & SKY_BACK)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));

    // if (renderFlag & SKY_DOWN)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));

    // if (renderFlag & SKY_FRONT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, -size));

    // glBindTextureif (renderFlag & SKY_LEFT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));

    // if (renderFlag & SKY_RIGHT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, -size));

    // if (renderFlag & SKY_UP)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));
}

void GenMapApp::RenderSky()
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    _skyShader.use();

    _skyShader.setupMatrices(_projectionMatrix * (_cam.GetViewMatrix() * glm::rotate(glm::translate(glm::mat4(1.0f), _cam.Position()), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))));

    _skyVertexBuffer.bind();
    glActiveTexture(GL_TEXTURE0);

    for (int i = 0; i < SkyTextures::Count; i++)
    {
        glBindTexture(GL_TEXTURE_2D, _skyTextureIndices[i]);

        glDrawArrays(GL_TRIANGLE_STRIP, i * 4, 4);
    }

    _skyVertexBuffer.unbind();
}

void OpenGLMessageCallback(
    unsigned source,
    unsigned type,
    unsigned id,
    unsigned severity,
    int length,
    const char *message,
    const void *userParam)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)userParam;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            spdlog::critical("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            spdlog::error("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            spdlog::warn("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            spdlog::trace("{} - {}", source, message);
            return;
    }

    spdlog::debug("Unknown severity level!");
    spdlog::debug(message);
}

void EnableOpenGlDebug()
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLMessageCallback, nullptr);

    glDebugMessageControl(
        GL_DONT_CARE,
        GL_DONT_CARE,
        GL_DEBUG_SEVERITY_NOTIFICATION,
        0,
        NULL,
        GL_FALSE);
}

// pos x, y, z, color h, s, v
float GenMapApp::vertices[216] = {
    -0.5f,
    -0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.4f,
    0.6f,
    1.0f,

    -0.5f,
    -0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.4f,
    0.6f,
    1.0f,

    -0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,

    0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.5f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    1.0f,

    -0.5f,
    -0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,

    -0.5f,
    0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.6f,
    0.6f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.6f,
    0.6f,
    1.0f,
};

const char *trailVertexShader = GLSL(
    in vec3 a_vertex;
    in vec3 a_color;
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec3 v_color;
    out vec2 v_texCoord;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex, 1.0);
        v_color = a_color;
        v_texCoord = a_texcoords.xy;
    });

const char *trailFragmentShader = GLSL(
    in vec3 v_color;
    in vec2 v_texCoord;

    out vec4 color;

    void main() {
        color = vec4(v_texCoord, 1.0f, 1.0) * vec4(v_color, 1.0f);
    });
