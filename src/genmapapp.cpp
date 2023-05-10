#include "genmapapp.h"

#include "valve/mdl/hl1mdlasset.h"
#include <application.h>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stb_image.h>

#define GLSL(src) "#version 330 core\n" #src

inline bool ends_with(
    std::string const &value,
    std::string const &ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

valve::Asset *AssetManager::LoadAsset(
    const std::string &assetName)
{
    auto found = _loadedAssets.find(assetName);

    if (found != _loadedAssets.end())
    {
        return found->second.get();
    }

    valve::Asset *asset = nullptr;

    if (ends_with(assetName, ".bsp"))
    {
        asset = new valve::hl1::BspAsset(&_fs);
    }

    if (ends_with(assetName, ".mdl"))
    {
        asset = new valve::hl1::MdlAsset(&_fs);
    }

    if (asset != nullptr && asset->Load(assetName))
    {
        _loadedAssets.insert(std::make_pair(assetName, asset));

        return asset;
    }

    return nullptr;
}

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

unsigned int UploadToGl(
    valve::Texture *texture)
{
    GLuint format = GL_RGB;
    GLuint glIndex = 0;

    switch (texture->Bpp())
    {
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
    }

    glGenTextures(1, &glIndex);
    glBindTexture(GL_TEXTURE_2D, glIndex);

    if (texture->Repeat())
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, texture->Width(), texture->Height(), 0, format, GL_UNSIGNED_BYTE, texture->Data());
    glGenerateMipmap(GL_TEXTURE_2D);

    return glIndex;
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

const char *solidBlendingVertexShader = GLSL(
    in vec3 a_vertex;
    in vec3 a_color;
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec3 f_color;
    out vec2 f_uv_tex;
    out vec2 f_uv_light;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex.xyz, 1.0);
        f_color = a_color;
        f_uv_light = a_texcoords.xy;
        f_uv_tex = a_texcoords.zw;
    });

const char *solidBlendingFragmentShader = GLSL(
    uniform sampler2D u_tex0;
    uniform sampler2D u_tex1;

    in vec3 f_color;
    in vec2 f_uv_tex;
    in vec2 f_uv_light;

    out vec4 color;

    void main() {
        vec4 texel0;
        vec4 texel1;
        texel0 = texture2D(u_tex0, f_uv_tex);
        texel1 = texture2D(u_tex1, f_uv_light);
        vec4 tempcolor = texel0 * texel1;
        if (texel0.a < 0.2)
            discard;
        else
            tempcolor = vec4(texel0.rgb, 1.0) * vec4(texel1.rgb, 1.0) * vec4(f_color, 1.0);
        color = tempcolor;
    });

const char *skyVertexShader = GLSL(
    in vec3 a_vertex;
    in vec3 a_color;
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec3 vcolor;
    out vec2 texCoord;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex, 1.0);
        vcolor = a_color;
        texCoord = a_texcoords.xy;
    });

const char *skyFragmentShader = GLSL(
    in vec3 vcolor;
    in vec2 texCoord;

    uniform sampler2D tex;

    out vec4 color;

    void main() {
        color = texture(tex, texCoord) * vec4(vcolor, 1.0f);
    });

const char *trailVertexShader = GLSL(
    in vec3 a_vertex;
    in vec3 a_color;
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec3 vcolor;
    out vec2 texCoord;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex, 1.0);
        vcolor = a_color;
        texCoord = a_texcoords.xy;
    });

const char *trailFragmentShader = GLSL(
    in vec3 vcolor;
    in vec2 texCoord;

    out vec4 color;

    void main() {
        color = vec4(texCoord, 1.0f, 1.0) * vec4(vcolor, 1.0f);
    });

bool GenMapApp::Startup()
{
    _physics = new PhysicsService();

    spdlog::debug("Startup()");

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

    glClearColor(0.0f, 0.45f, 0.7f, 1.0f);

    spdlog::info("{} @ {}", _assets._fs.Mod(), _assets._fs.Root().generic_string());

    _bspAsset = _assets.LoadAsset<valve::hl1::BspAsset>(_map);

    if (_bspAsset == nullptr)
    {
        return false;
    }

    SetupBsp();

    _normalBlendingShader.compileDefaultShader();
    _solidBlendingShader.compile(solidBlendingVertexShader, solidBlendingFragmentShader);

    _vertexBuffer
        .setup(_normalBlendingShader);

    SetupSky();

    _skyShader.compile(skyVertexShader, skyFragmentShader);
    _skyVertexBuffer.setup(_skyShader);

    auto origin = SetupEntities();

    _trailShader.compile(trailVertexShader, trailFragmentShader);

    for (int v = 0; v < 36; v++)
    {
        _vertexArray
            .vertex_and_col(&vertices[v * 6], glm::vec3(10.0f));
    }

    _vertexArray.setup(_trailShader);

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

std::vector<PhysicsComponent> balls;
bool showPhysicsDebug = false;

bool GenMapApp::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    _physics->Step(time);

    const float speed = 8.0f;

    if (IsMouseButtonPushed(inputState, MouseButtons::LeftButton))
    {
        auto comp = _physics->AddCube(1.0f, glm::vec3(30.0f), _cam.Position() + (_cam.Forward() * 40.0f));
        _physics->ApplyForce(comp, glm::normalize(_cam.Forward()) * 7000.0f);
        balls.push_back(comp);
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

    _cam.ProcessMouseMovement(inputState.MousePointerPosition[0], inputState.MousePointerPosition[1], true);

    auto m = _physics->GetMatrix(_character);
    _cam.SetPosition(glm::vec3(m[3]) + glm::vec3(0.0f, 0.0f, 40.0f));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSky();
    RenderBsp();

    glDisable(GL_CULL_FACE);

    _vertexArray.bind();

    for (auto &ball : balls)
    {
        auto m = glm::scale(_physics->GetMatrix(ball), glm::vec3(3.0f));

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

void GenMapApp::SetupSky()
{
    // here we make up for the half of pixel to get the sky textures really stitched together because clamping is not enough
    const float uv_1 = 255.0f / 256.0f;
    const float uv_0 = 1.0f - uv_1;
    const float size = 1.0f;

    // if (renderFlag & SKY_BACK)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, size));

    // if (renderFlag & SKY_DOWN)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, -size, size));

    // if (renderFlag & SKY_FRONT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));

    // glBindTextureif (renderFlag & SKY_LEFT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(-size, -size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));

    // if (renderFlag & SKY_RIGHT)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, -size, size));

    // if (renderFlag & SKY_UP)
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, -size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_0, 0, 0))
        .vertex(glm::vec3(-size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_0, uv_1, 0, 0))
        .vertex(glm::vec3(size, size, size));
    _skyVertexBuffer
        .uvs(glm::vec4(uv_1, uv_1, 0, 0))
        .vertex(glm::vec3(size, size, -size));
}

void GenMapApp::SetupBsp()
{
    _lightmapIndices = std::vector<GLuint>(_bspAsset->_lightMaps.size());
    glActiveTexture(GL_TEXTURE1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (size_t i = 0; i < _bspAsset->_lightMaps.size(); i++)
    {
        _lightmapIndices[i] = UploadToGl(_bspAsset->_lightMaps[i]);
    }

    _textureIndices = std::vector<GLuint>(_bspAsset->_textures.size());
    glActiveTexture(GL_TEXTURE0);
    for (size_t i = 0; i < _bspAsset->_textures.size(); i++)
    {
        _textureIndices[i] = UploadToGl(_bspAsset->_textures[i]);
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
                    .vertex(glm::vec3(vertex.position));
            }
        }

        _faces.push_back(ft);
    }

    for (int i = 0; i < 6; i++)
    {
        _skyTextureIndices[i] = UploadToGl(_bspAsset->_skytextures[i]);
    }

    spdlog::debug("loaded {0} vertices", _vertexBuffer.vertexCount());
}

glm::vec3 GenMapApp::SetupEntities()
{
    for (auto &bspEntity : _bspAsset->_entities)
    {
        const auto entity = _registry.create();

        // spdlog::info("Entity {}", bspEntity.classname);
        // for (auto kvp : bspEntity.keyvalues)
        // {
        //     spdlog::info("    {} = {}", kvp.first, kvp.second);
        // }
        // spdlog::info(" ");

        if (bspEntity.classname == "worldspawn")
        {
            _registry.assign<ModelComponent>(entity, 0);
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
            ModelComponent mc = {0};

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

                if (asset != nullptr)
                {
                    StudioComponent sc = {
                        .AssetId = asset->Id(),
                    };

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

        auto origin = bspEntity.keyvalues.find("origin");
        if (origin != bspEntity.keyvalues.end())
        {
            glm::vec3 originPosition;

            std::istringstream(origin->second) >> originPosition.x >> originPosition.y >> originPosition.z;

            _registry.assign<OriginComponent>(entity, originPosition);
        }
        else
        {
            _registry.assign<OriginComponent>(entity, glm::vec3(0.0f));
        }
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

void GenMapApp::Resize(
    int width,
    int height)
{
    glViewport(0, 0, width, height);

    _projectionMatrix = glm::perspective(glm::radians(90.0f), float(width) / float(height), 0.1f, 4096.0f);
}

void GenMapApp::Destroy()
{
    if (_bspAsset != nullptr)
    {
        delete _bspAsset;
        _bspAsset = nullptr;
    }
}

void GenMapApp::RenderSky()
{
    glEnable(GL_TEXTURE_2D);
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

        glDrawArrays(GL_QUADS, i * 4, 4);
    }

    _skyVertexBuffer.unbind();
}

void GenMapApp::RenderBsp()
{
    _vertexBuffer.bind();

    glEnable(GL_DEPTH_TEST);

    auto m = _projectionMatrix * _cam.GetViewMatrix();

    glDisable(GL_BLEND);
    RenderModelsByRenderMode(RenderModes::NormalBlending, _normalBlendingShader, m);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_DST_ALPHA);
    RenderModelsByRenderMode(RenderModes::TextureBlending, _normalBlendingShader, m);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderModelsByRenderMode(RenderModes::SolidBlending, _solidBlendingShader, m);

    _vertexBuffer.unbind();
}

void GenMapApp::RenderModelsByRenderMode(
    RenderModes mode,
    ShaderType &shader,
    const glm::mat4 &matrix)
{
    auto view = _registry.view<RenderComponent, ModelComponent, OriginComponent>();

    shader.use();

    if (mode == RenderModes::NormalBlending)
    {
        shader.setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    for (auto entity : view)
    {
        auto renderComponent = _registry.get<RenderComponent>(entity);

        if (renderComponent.Mode != mode)
        {
            continue;
        }

        if (mode == RenderModes::TextureBlending || mode == RenderModes::SolidBlending)
        {
            shader.setupColor(
                glm::vec4(1.0f, 1.0f, 1.0f, float(renderComponent.Amount) / 255.0f));
        }

        auto modelComponent = _registry.get<ModelComponent>(entity);
        auto originComponent = _registry.get<OriginComponent>(entity);

        shader.setupMatrices(glm::translate(matrix, originComponent.Origin));

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
