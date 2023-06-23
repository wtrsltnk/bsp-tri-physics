#include "engine.hpp"

#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <valve/mdl/hl1mdlinstance.h>

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

Engine::Engine(
    IRenderer *renderer,
    IPhysicsService *physicsService,
    IAssetManager *assetManager)
    : _renderer(renderer),
      _physicsService(physicsService),
      _assetManager(assetManager)
{}

Engine::~Engine() = default;

void Engine::SetProjectionMatrix(
    const glm::mat4 &projectionMatrix)
{
    _projectionMatrix = projectionMatrix;
}

bool Engine::Load(
    const std::string &asset)
{
    auto rootAsset = _assetManager->LoadAsset(asset);

    if (rootAsset == nullptr)
    {
        spdlog::error("Failed to load {}", asset);

        return false;
    }

    if (rootAsset->AssetType() == valve::AssetTypes::Spr)
    {
        sprAsset = dynamic_cast<valve::hl1::SprAsset *>(rootAsset);

        const auto entity = _registry.create();

        OriginComponent originComponent = {
            .Origin = glm::vec3(0.0f),
            .Angles = glm::vec3(0.0f),
        };

        _registry.assign<OriginComponent>(entity, originComponent);

        RenderComponent rc = {
            .Amount = 0,
            .Color = {255, 255, 255},
            .Mode = RenderModes::NormalBlending,
        };

        _registry.assign<RenderComponent>(entity, rc);

        _registry.assign<SpriteComponent>(entity, BuildSpriteComponent(sprAsset));
    }
    else if (rootAsset->AssetType() == valve::AssetTypes::Mdl)
    {
        mdlAsset = dynamic_cast<valve::hl1::MdlAsset *>(rootAsset);

        auto center = mdlAsset->_header->min + ((mdlAsset->_header->max - mdlAsset->_header->min) * 0.5f);

        const auto entity = _registry.create();

        OriginComponent originComponent = {
            .Origin = glm::vec3(0.0f),
            .Angles = glm::vec3(0.0f),
        };

        _registry.assign<OriginComponent>(entity, originComponent);

        RenderComponent rc = {
            .Amount = 0,
            .Color = {255, 255, 255},
            .Mode = RenderModes::NormalBlending,
        };

        _registry.assign<RenderComponent>(entity, rc);

        _registry.assign<StudioComponent>(entity, BuildStudioComponent(mdlAsset));

        auto offset = glm::length(center);
        if (offset == 0.0f)
        {
            offset = 50.0f;
        }

        _cam.SetPosition(center + glm::vec3(0.0f, offset, 0.0f));
    }
    else if (rootAsset->AssetType() == valve::AssetTypes::Bsp)
    {
        bspAsset = dynamic_cast<valve::hl1::BspAsset *>(rootAsset);

        if (!SetupBsp(bspAsset))
        {
            return false;
        }

        auto entities = _registry.view<PlayerStartComponent, OriginComponent>();

        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        if (!entities.empty())
        {
            auto size = entities.size();
            auto randomMax = std::rand() % size;

            size_t i = 0;
            for (auto &entity : entities)
            {
                i++;
                if (i >= randomMax)
                {
                    auto originComponent = _registry.try_get<OriginComponent>(entity);

                    // Todo, use angles for character look direction at spawn
                    _character = _physicsService->AddCharacter(15, 16, 45, originComponent->Origin);

                    break;
                }
            }
        }
    }

    if (!_vertexBuffer.upload())
    {
        spdlog::error("failed to upload vertex data");

        return false;
    }

    _defaultShader = _renderer->LoadShader("");

    if (_defaultShader == nullptr)
    {
        return false;
    }

    valve::Texture lm;
    lm.SetDimentions(32, 32, 4);
    lm.Fill(glm::vec4(255, 255, 255, 255));
    _emptyWhiteTexture = _renderer->LoadTexture(32, 32, 4, false, lm.Data());

    return true;
}

StudioComponent Engine::BuildStudioComponent(
    valve::hl1::MdlAsset *mdlAsset,
    float scale)
{
    StudioComponent sc = {
        .AssetId = mdlAsset->Id(),
        .Scale = scale,
        .FirstVertexInBuffer = static_cast<int>(_vertexBuffer.vertexCount()),
        .VertexCount = static_cast<int>(mdlAsset->_vertices.size()),
    };

    sc.LightmapOffset = static_cast<int>(_lightmapIndices.size());
    for (size_t i = 0; i < mdlAsset->_lightmaps.size(); i++)
    {
        auto &tex = mdlAsset->_lightmaps[i];

        auto textureIndex = _renderer->LoadLightmap(
            tex->Width(),
            tex->Height(),
            tex->Bpp(),
            tex->Repeat(),
            tex->Data());

        _lightmapIndices.push_back(textureIndex);
    }

    sc.TextureOffset = static_cast<int>(_textureIndices.size());
    for (size_t i = 0; i < mdlAsset->_textures.size(); i++)
    {
        auto &tex = mdlAsset->_textures[i];

        auto textureIndex = _renderer->LoadTexture(
            tex->Width(),
            tex->Height(),
            tex->Bpp(),
            tex->Repeat(),
            tex->Data());

        _textureIndices.push_back(textureIndex);
    }

    for (auto &vert : mdlAsset->_vertices)
    {
        _vertexBuffer
            .bone(vert.bone)
            .col(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
            .uvs(vert.texcoords)
            .vertex(vert.position);
    }

    return sc;
}

SpriteComponent Engine::BuildSpriteComponent(
    valve::hl1::SprAsset *sprAsset,
    float scale)
{
    SpriteComponent sc = {
        .AssetId = sprAsset->Id(),
        .Scale = scale,
        .FirstVertexInBuffer = static_cast<int>(_vertexBuffer.vertexCount()),
        .VertexCount = static_cast<int>(sprAsset->_vertices.size()),
    };

    sc.TextureOffset = static_cast<int>(_textureIndices.size());
    for (size_t i = 0; i < sprAsset->_textures.size(); i++)
    {
        auto &tex = sprAsset->_textures[i];

        auto textureIndex = _renderer->LoadTexture(
            tex->Width(),
            tex->Height(),
            tex->Bpp(),
            tex->Repeat(),
            tex->Data());

        _textureIndices.push_back(textureIndex);
    }

    for (auto &vert : sprAsset->_vertices)
    {
        _vertexBuffer
            .bone(-1)
            .col(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
            .uvs(vert.texcoords)
            .vertex(vert.position);
    }

    return sc;
}

bool Engine::SetupBsp(
    valve::hl1::BspAsset *bspAsset)
{
    _lightmapIndices = std::vector<GLuint>();
    for (size_t i = 0; i < bspAsset->_lightMaps.size(); i++)
    {
        auto &tex = bspAsset->_lightMaps[i];

        auto textureIndex = _renderer->LoadLightmap(
            tex->Width(),
            tex->Height(),
            tex->Bpp(),
            tex->Repeat(),
            tex->Data());

        _lightmapIndices.push_back(textureIndex);
    }

    _textureIndices = std::vector<GLuint>();
    for (size_t i = 0; i < bspAsset->_textures.size(); i++)
    {
        auto &tex = bspAsset->_textures[i];

        auto textureIndex = _renderer->LoadTexture(
            tex->Width(),
            tex->Height(),
            tex->Bpp(),
            tex->Repeat(),
            tex->Data());

        _textureIndices.push_back(textureIndex);
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
                    .bone(-1)
                    .vertex(glm::vec3(vertex.position));
            }
        }

        _faces.push_back(ft);
    }

    if (!SetupEntities(bspAsset))
    {
        return false;
    }

    return true;
}

bool Engine::SetupEntities(
    valve::hl1::BspAsset *bspAsset)
{
    std::vector<glm::vec3> triangles;

    for (auto &bspEntity : bspAsset->_entities)
    {
        const auto entity = _registry.create();

        if (bspEntity.classname == "worldspawn")
        {
            GrabTriangles(bspAsset, 0, triangles);

            _registry.assign<ModelComponent>(entity, 0);

            SetupSky(bspAsset);

            RenderComponent rc = {
                .Amount = 255,
                .Color = {255, 255, 255},
                .Mode = RenderModes::NormalBlending,
            };

            _registry.assign<RenderComponent>(entity, rc);

            OriginComponent oc = {
                .Origin = glm::vec3(0.0f),
                .Angles = glm::vec3(0.0f),
            };

            _registry.assign<OriginComponent>(entity, oc);

            continue;
        }

        if (bspEntity.classname == "info_player_start" ||
            bspEntity.classname == "info_player_deathmatch" ||
            bspEntity.classname == "info_player_coop")
        {
            auto originComponent = BuildOriginComponent(bspEntity);
            _registry.assign<OriginComponent>(entity, originComponent);

            PlayerStartComponent playerStartComponent = {
                .className = bspEntity.classname,
            };
            _registry.assign<PlayerStartComponent>(entity, playerStartComponent);

            continue;
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

                if (bspEntity.classname.rfind("func_wall", 0) == 0 ||
                    bspEntity.classname.rfind("func_breakable", 0) == 0 ||
                    bspEntity.classname.rfind("func_plat", 0) == 0)
                {
                    GrabTriangles(bspAsset, mc.Model, triangles);
                }
            }
            else
            {
                float scale = 1.0f;
                if (bspEntity.keyvalues.count("scale") != 0)
                {
                    scale = std::stof(bspEntity.keyvalues["scale"]);
                }

                auto asset = _assetManager->LoadAsset(bspEntity.keyvalues["model"]);

                auto sprAsset = dynamic_cast<valve::hl1::SprAsset *>(asset);
                auto mdlAsset = dynamic_cast<valve::hl1::MdlAsset *>(asset);

                if (sprAsset != nullptr)
                {
                    _registry.assign<SpriteComponent>(entity, BuildSpriteComponent(sprAsset, scale));
                }
                else if (mdlAsset != nullptr)
                {
                    _registry.assign<StudioComponent>(entity, BuildStudioComponent(mdlAsset, scale));
                }
            }
        }

        RenderComponent rc = {
            .Amount = 0,
            .Color = {255, 255, 255},
            .Mode = RenderModes::NormalBlending,
        };

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

        auto originComponent = BuildOriginComponent(bspEntity);

        _registry.assign<OriginComponent>(entity, originComponent);
    }

    _registry.sort<RenderComponent>([](const RenderComponent &lhs, const RenderComponent &rhs) {
        return lhs.Mode < rhs.Mode;
    });

    _physicsService->AddStatic(triangles);

    return true;
}

OriginComponent Engine::BuildOriginComponent(
    valve::hl1::tBSPEntity &bspEntity)
{
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

    return originComponent;
}

void Engine::SetupSky(
    valve::hl1::BspAsset *bspAsset)
{
    for (int i = 0; i < 6; i++)
    {
        auto &tex = bspAsset->_skytextures[i];
        if (tex == nullptr)
        {
            continue;
        }
        _skyTextureIndices[i] = _renderer->LoadTexture(tex->Width(), tex->Height(), tex->Bpp(), tex->Repeat(), tex->Data());
    }

    // here we make up for the half of pixel to get the sky textures really stitched together because clamping is not enough
    const float uv_1 = 255.0f / 256.0f;
    const float uv_0 = 1.0f - uv_1;
    const float size = 1.0f;

    _firstSkyVertex = _vertexBuffer.vertexCount();
    // if (renderFlag & SKY_BACK)
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(size, size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(-size, size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(-size, -size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(size, -size, size));

    // if (renderFlag & SKY_DOWN)
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(size, -size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(-size, -size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(-size, -size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(size, -size, -size));

    // if (renderFlag & SKY_FRONT)
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(-size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(size, -size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(-size, -size, -size));

    // glBindTextureif (renderFlag & SKY_LEFT)
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(-size, size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(-size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(-size, -size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(-size, -size, size));

    // if (renderFlag & SKY_RIGHT)
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(size, -size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(size, -size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(size, size, size));

    // if (renderFlag & SKY_UP)
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_1))
        .vertex(glm::vec3(size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_1, uv_0))
        .vertex(glm::vec3(-size, size, -size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_0))
        .vertex(glm::vec3(-size, size, size));
    _vertexBuffer
        .uvs(glm::vec2(uv_0, uv_1))
        .vertex(glm::vec3(size, size, size));
}

void Engine::GrabTriangles(
    valve::hl1::BspAsset *bspAsset,
    int moddelIndex,
    std::vector<glm::vec3> &triangles)
{
    auto &model = bspAsset->_models[moddelIndex];

    for (int f = model.firstFace; f < model.firstFace + model.faceCount; f++)
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
}

void Engine::Update(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    _physicsService->Step(time);

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
}

void Engine::HandleBspInput(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    (void)time;

    const float speed = 8.0f;

    if (IsKeyboardButtonPushed(inputState, KeyboardButtons::KeySpace))
    {
        _physicsService->JumpCharacter(_character, _cam.Up());
    }

    if (inputState.KeyboardButtonStates[KeyboardButtons::KeyLeft] || inputState.KeyboardButtonStates[KeyboardButtons::KeyA])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            _physicsService->MoveCharacter(_character, _cam.Forward() + _cam.Left(), speed);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            _physicsService->MoveCharacter(_character, _cam.Back() + _cam.Left(), speed);
        }
        else
        {
            _physicsService->MoveCharacter(_character, _cam.Left(), speed);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyRight] || inputState.KeyboardButtonStates[KeyboardButtons::KeyD])
    {
        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            _physicsService->MoveCharacter(_character, _cam.Forward() + _cam.Right(), speed);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            _physicsService->MoveCharacter(_character, _cam.Back() + _cam.Right(), speed);
        }
        else
        {
            _physicsService->MoveCharacter(_character, _cam.Right(), speed);
        }
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
    {
        _physicsService->MoveCharacter(_character, _cam.Forward(), speed);
    }
    else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
    {
        _physicsService->MoveCharacter(_character, _cam.Back(), speed);
    }
    else
    {
        _physicsService->MoveCharacter(_character, glm::vec3(0.0f), speed);
    }

    auto m = _physicsService->GetMatrix(_character);
    _cam.SetPosition(glm::vec3(m[3]) + glm::vec3(0.0f, 0.0f, 32.0f));
}

void Engine::HandleMdlInput(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    float speed = _character.bodyIndex > 0 ? 200.0f : 40.0f;

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

bool Engine::Render(
    std::chrono::microseconds time)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    if (sprAsset != nullptr)
    {
        glDepthFunc(GL_LEQUAL);

        glDisable(GL_BLEND);

        RenderSpritesByRenderMode(RenderModes::NormalBlending, time);

        return true;
    }
    else if (mdlAsset != nullptr)
    {
        RenderStudioModelsByRenderMode(RenderModes::NormalBlending, time);

        return true;
    }
    else if (bspAsset != nullptr)
    {
        RenderBsp(bspAsset, time);

        return true;
    }

    return false;
}

void Engine::RenderBsp(
    valve::hl1::BspAsset *bspAsset,
    std::chrono::microseconds time)
{
    RenderSky();

    glDisable(GL_BLEND);
    RenderByRenderMode(bspAsset, RenderModes::NormalBlending, time);

    RenderModelsByRenderMode(bspAsset, RenderModes::ColorBlending);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    RenderByRenderMode(bspAsset, RenderModes::AdditiveBlending, time);
    RenderByRenderMode(bspAsset, RenderModes::TextureBlending, time);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderByRenderMode(bspAsset, RenderModes::SolidBlending, time);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    RenderSpritesByRenderMode(RenderModes::GlowBlending, time);
}

void Engine::RenderByRenderMode(
    valve::hl1::BspAsset *bspAsset,
    RenderModes mode,
    std::chrono::microseconds time)
{
    RenderModelsByRenderMode(bspAsset, mode);
    RenderStudioModelsByRenderMode(mode, time);
    RenderSpritesByRenderMode(mode, time);
}

void Engine::RenderModelsByRenderMode(
    valve::hl1::BspAsset *bspAsset,
    RenderModes mode)
{
    if (mode == RenderModes::GlowBlending)
    {
        return;
    }

    auto entities = _registry.view<RenderComponent, ModelComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    _defaultShader->use();
    _defaultShader->setupSpriteType(9);
    _defaultShader->setupBrightness(0.2f);

    for (auto entity : entities)
    {
        if (!SetupRenderComponent(entity, mode))
        {
            continue;
        }

        if (!SetupOriginComponent(entity))
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

            _renderer->BindTexture(_textureIndices[_faces[i].texture]);

            _renderer->BindLightmap(_lightmapIndices[_faces[i].lightmap]);

            glDrawArrays(GL_TRIANGLE_FAN, _faces[i].firstVertex, _faces[i].vertexCount);
        }
    }
}

void Engine::RenderSpritesByRenderMode(
    RenderModes mode,
    std::chrono::microseconds time)
{
    auto dt = float(double(time.count()) / 1000000.0);

    auto entities = _registry.view<RenderComponent, SpriteComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    _defaultShader->use();

    _defaultShader->setupBrightness(0.5f);
    _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    for (auto entity : entities)
    {
        auto spriteComponent = _registry.try_get<SpriteComponent>(entity);

        auto asset = _assetManager->GetAsset<valve::hl1::SprAsset>(spriteComponent->AssetId);

        if (asset == nullptr)
        {
            continue;
        }

        if (!SetupRenderComponent(entity, mode))
        {
            continue;
        }

        if (!SetupOriginComponent(entity, spriteComponent->Scale))
        {
            continue;
        }

        _defaultShader->setupSpriteType(asset->_type);
        _vertexBuffer.bind();

        spriteComponent->Frame += (dt * 24.0f);

        if (size_t(spriteComponent->Frame) >= asset->_faces.size())
        {
            spriteComponent->Frame = 0;
        }

        auto &face = asset->_faces[size_t(spriteComponent->Frame)];

        _renderer->BindTexture(_textureIndices[spriteComponent->TextureOffset + face.texture]);

        _renderer->BindLightmap(_emptyWhiteTexture);

        glDrawArrays(GL_TRIANGLE_FAN, spriteComponent->FirstVertexInBuffer + face.firstVertex, face.vertexCount);
    }
}

void Engine::RenderStudioModelsByRenderMode(
    RenderModes mode,
    std::chrono::microseconds time)
{
    if (mode == RenderModes::GlowBlending)
    {
        return;
    }

    auto entities = _registry.view<RenderComponent, StudioComponent, OriginComponent>();

    if (entities.empty())
    {
        return;
    }

    _defaultShader->use();
    _defaultShader->setupSpriteType(9);
    _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    _defaultShader->setupBrightness(0.5f);

    valve::hl1::MdlInstance _mdlInstance;
    for (auto entity : entities)
    {
        auto studioComponent = _registry.try_get<StudioComponent>(entity);

        auto asset = _assetManager->GetAsset<valve::hl1::MdlAsset>(studioComponent->AssetId);

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

        _defaultShader->BindBones(_mdlInstance._bonetransform, _mdlInstance.Asset->_boneData.size());

        if (!SetupRenderComponent(entity, mode))
        {
            continue;
        }

        if (!SetupOriginComponent(entity, studioComponent->Scale))
        {
            continue;
        }

        _vertexBuffer.bind();

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

            _renderer->BindTexture(_textureIndices[studioComponent->TextureOffset + face.texture]);

            _renderer->BindLightmap(_emptyWhiteTexture);

            glDrawArrays(GL_TRIANGLES, studioComponent->FirstVertexInBuffer + face.firstVertex, face.vertexCount);
        }

        _defaultShader->UnbindBones();
    }
}

bool Engine::SetupRenderComponent(
    const entt::entity &entity,
    RenderModes mode)
{
    auto renderComponent = _registry.get<RenderComponent>(entity);

    if (renderComponent.Mode != mode)
    {
        return false;
    }

    if (mode == RenderModes::TextureBlending || mode == RenderModes::SolidBlending)
    {
        _defaultShader->setupColor(
            glm::vec4(1.0f, 1.0f, 1.0f, float(renderComponent.Amount) / 255.0f));
    }
    else if (mode == RenderModes::ColorBlending)
    {
        _defaultShader->setupColor(
            glm::vec4(
                float(renderComponent.Color[0] / 255.0f),
                float(renderComponent.Color[1] / 255.0f),
                float(renderComponent.Color[2] / 255.0f),
                float(renderComponent.Amount) / 255.0f));
    }
    else
    {
        _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    return true;
}

bool Engine::SetupOriginComponent(
    const entt::entity &entity,
    float scale)
{
    auto originComponent = _registry.get<OriginComponent>(entity);

    auto modelMatrix = glm::translate(glm::mat4(1.0f), originComponent.Origin);

    modelMatrix = glm::rotate(modelMatrix, glm::radians(originComponent.Angles.z), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(originComponent.Angles.y), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(originComponent.Angles.x), glm::vec3(0.0f, -1.0f, 0.0f));

    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, scale));

    _defaultShader->setupMatrices(_projectionMatrix, _cam.GetViewMatrix(), modelMatrix);

    return true;
}

void Engine::RenderSky()
{
    _renderer->DisableDepthTesting();

    _defaultShader->use();

    _defaultShader->setupSpriteType(9);
    _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    _defaultShader->setupMatrices(
        _projectionMatrix,
        _cam.GetViewMatrix(),
        glm::rotate(glm::translate(glm::mat4(1.0f), _cam.Position()), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    _vertexBuffer.bind();

    _renderer->BindLightmap(_emptyWhiteTexture);

    for (int i = 0; i < SkyTextures::Count; i++)
    {
        _renderer->BindTexture(_skyTextureIndices[i]);

        glDrawArrays(GL_TRIANGLE_FAN, _firstSkyVertex + (i * 4), 4);
    }

    _renderer->EnableDepthTesting();
}
