#include "genmapapp.h"

#include "renderers/opengl/openglrenderer.hpp"
#include "valve/mdl/hl1mdlasset.h"
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

    _bspAsset = _assets.LoadAsset<valve::hl1::BspAsset>(_map);

    if (_bspAsset == nullptr)
    {
        return false;
    }

    auto origin = SetupBsp();

    _trailShader.compile(trailVertexShader, trailFragmentShader);

    for (int v = 0; v < 36; v++)
    {
        _vertexArray
            .vertex_and_col(&vertices[v * 6], glm::vec3(10.0f));
    }

    _vertexArray.upload(_trailShader);

    std::vector<glm::vec3> triangles;

    auto &rootModel = _bspAsset->_models[0];

    for (int f = rootModel.firstFace; f < rootModel.firstFace + rootModel.faceCount; f++)
    {
        auto &face = _bspAsset->_faces[f];

        // if (face.flags != 0)
        // {
        //     continue;
        // }

        auto &vertex1 = _bspAsset->_vertices[face.firstVertex];
        auto &vertex2 = _bspAsset->_vertices[face.firstVertex + 1];

        for (int v = face.firstVertex + 2; v < face.firstVertex + face.vertexCount; v++)
        {
            triangles.push_back(vertex1.position);
            triangles.push_back(vertex2.position);
            triangles.push_back(_bspAsset->_vertices[v].position);

            vertex2 = _bspAsset->_vertices[v];
        }
    }

    _physics->AddStatic(triangles);

    _character = _physics->AddCharacter(15, 16, 45, origin);

    return true;
}

bool showPhysicsDebug = false;

bool GenMapApp::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    _physics->Step(time);

    const float speed = 8.0f;

    if (IsMouseButtonPushed(inputState, MouseButtons::LeftButton))
    {
        const auto entity = _registry.create();

        auto comp = _physics->AddCube(1.0f, glm::vec3(30.0f), _cam.Position() + (_cam.Forward() * 40.0f));

        _physics->ApplyForce(comp, glm::normalize(_cam.Forward()) * 7000.0f);

        _registry.assign<PhysicsComponent>(entity, comp);
        _registry.assign<BallComponent>(entity, 0);
    }

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

    _cam.ProcessMouseMovement(
        float(inputState.MousePointerPosition[0]),
        float(inputState.MousePointerPosition[1]),
        true);

    auto m = _physics->GetMatrix(_character);
    _cam.SetPosition(glm::vec3(m[3]) + glm::vec3(0.0f, 0.0f, 40.0f));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSky();
    RenderBsp(time);

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

void GenMapApp::SetFilename(
    const char *root,
    const char *map)
{
    _assets._fs.FindRootFromFilePath(root);
    _map = map;
}

glm::vec3 GenMapApp::SetupBsp()
{
    _lightmapIndices = std::vector<GLuint>();
    glActiveTexture(GL_TEXTURE1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (size_t i = 0; i < _bspAsset->_lightMaps.size(); i++)
    {
        auto &tex = _bspAsset->_lightMaps[i];
        _lightmapIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    _textureIndices = std::vector<GLuint>();
    glActiveTexture(GL_TEXTURE0);
    for (size_t i = 0; i < _bspAsset->_textures.size(); i++)
    {
        auto &tex = _bspAsset->_textures[i];
        _textureIndices.push_back(_renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data()));
    }

    for (size_t f = 0; f < _bspAsset->_faces.size(); f++)
    {
        auto &face = _bspAsset->_faces[f];

        FaceType ft;

        ft.flags = face.flags;
        ft.firstVertex = 0;
        ft.vertexCount = 0;
        ft.lightmapIndex = 0;
        ft.textureIndex = 0;

        if (face.flags == 0)
        {
            ft.firstVertex = _vertexBuffer.vertexCount();
            ft.vertexCount = face.vertexCount;
            ft.lightmapIndex = static_cast<unsigned int>(f);
            ft.textureIndex = face.texture;

            for (int v = face.firstVertex; v < face.firstVertex + face.vertexCount; v++)
            {
                auto &vertex = _bspAsset->_vertices[v];

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

    auto origin = SetupEntities();

    _studioNormalBlendingShader.compileMdlShader();
    _studioVertexArray.upload(_studioNormalBlendingShader, true);

    return origin;
}

glm::vec3 GenMapApp::SetupEntities()
{
    for (auto &bspEntity : _bspAsset->_entities)
    {
        const auto entity = _registry.create();

        if (bspEntity.classname == "worldspawn")
        {
            _registry.assign<ModelComponent>(entity, 0);

            SetupSky();

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

        if (bspEntity.keyvalues.count("model") != 0 && bspEntity.classname.rfind("func_", 0) != 0 && bspEntity.classname.rfind("hostage_entity", 0) != 0)
        {
            ModelComponent mc = {
                .AssetId = _bspAsset->Id(),
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
                auto mdlAsset = _assets.LoadAsset<valve::hl1::MdlAsset>(bspEntity.keyvalues["model"]);

                if (mdlAsset != nullptr)
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

                    _registry.assign<StudioComponent>(entity, sc);
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
        if (angles != bspEntity.keyvalues.end())
        {
            glm::vec3 anglesPosition;

            std::istringstream(angles->second) >> anglesPosition.x >> anglesPosition.y >> anglesPosition.z;

            originComponent.Angles = anglesPosition;
        }

        _registry.assign<OriginComponent>(entity, originComponent);
    }

    auto info_player_start = _bspAsset->FindEntityByClassname("info_player_start");

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
    std::chrono::microseconds time)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    auto m = _projectionMatrix * _cam.GetViewMatrix();

    glDisable(GL_BLEND);
    RenderModelsByRenderMode(RenderModes::NormalBlending, _normalBlendingShader, m);
    RenderStudioModelsByRenderMode(RenderModes::NormalBlending, _studioNormalBlendingShader, m, time);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_DST_ALPHA);
    RenderModelsByRenderMode(RenderModes::TextureBlending, _solidBlendingShader, m);
    RenderStudioModelsByRenderMode(RenderModes::TextureBlending, _studioNormalBlendingShader, m, time);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderModelsByRenderMode(RenderModes::SolidBlending, _solidBlendingShader, m);
    RenderStudioModelsByRenderMode(RenderModes::SolidBlending, _studioNormalBlendingShader, m, time);
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
        auto model = _bspAsset->_models[modelComponent.Model];

        for (int i = model.firstFace; i < model.firstFace + model.faceCount; i++)
        {
            if (_faces[i].flags > 0)
            {
                continue;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textureIndices[_faces[i].textureIndex]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _lightmapIndices[_faces[i].lightmapIndex]);

            glDrawArrays(GL_TRIANGLE_FAN, _faces[i].firstVertex, _faces[i].vertexCount);
        }
    }
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
    shader.setupBrightness(2.0f);

    if (mode == RenderModes::NormalBlending)
    {
        shader.setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

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

void GenMapApp::SetupSky()
{
    for (int i = 0; i < 6; i++)
    {
        auto &tex = _bspAsset->_skytextures[i];
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
