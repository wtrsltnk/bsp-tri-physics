#include "genmapapp.h"
#include <glad/glad.h>

#include "include/application.h"

#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stb_image.h>

#define GLSL(src) "#version 330 core\n" #src

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
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec2 f_uv_tex;
    out vec2 f_uv_light;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex.xyz, 1.0);
        f_uv_light = a_texcoords.xy;
        f_uv_tex = a_texcoords.zw;
    });

const char *solidBlendingFragmentShader = GLSL(
    uniform sampler2D u_tex0;
    uniform sampler2D u_tex1;

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
            tempcolor = vec4(texel0.rgb, 1.0) * vec4(texel1.rgb, 1.0);
        color = tempcolor;
    });

const char *skyVertexShader = GLSL(
    in vec3 a_vertex;
    in vec4 a_texcoords;

    uniform mat4 u_matrix;

    out vec2 texCoord;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex, 1.0);
        texCoord = a_texcoords.xy;
    });

const char *skyFragmentShader = GLSL(
    in vec2 texCoord;

    uniform sampler2D tex;

    out vec4 color;

    void main() {
        color = texture(tex, texCoord);
    });

const char *trailVertexShader = GLSL(
    in vec3 a_vertex;
    in vec3 a_color;

    uniform mat4 u_matrix;

    out vec3 cc;

    void main() {
        gl_Position = u_matrix * vec4(a_vertex, 1.0);
        cc = a_color;
    });

const char *trailFragmentShader = GLSL(
    in vec3 cc;

    out vec4 color;

    void main() {
        color = vec4(cc.xyz, 1.0);
    });

static bool _skipClipping = false;

// pos x, y, z, color h, s, v
static float vertices[216] = {
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

static std::tuple<size_t, size_t> mesh;

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

    spdlog::info("{} @ {}", _fs.Mod(), _fs.Root().generic_string());

    _bspAsset = new valve::hl1::BspAsset(&_fs);
    if (_bspAsset->Load(_map))
    {
        SetupBsp();
    }

    auto info_player_start = _bspAsset->FindEntityByClassname("info_player_start");

    glm::vec3 angles(0.0f);
    glm::vec3 origin(0.0f);
    if (info_player_start != nullptr)
    {
        std::istringstream(info_player_start->keyvalues["angles"]) >> (angles.x) >> (angles.y) >> (angles.z);
        _cam.RotateX(angles.x);
        _cam.RotateY(angles.y);
        _cam.RotateZ(angles.z);

        std::istringstream(info_player_start->keyvalues["origin"]) >> (origin.x) >> (origin.y) >> (origin.z);
        _cam.SetPosition(origin);
    }

    SetupSky();

    _registry.sort<RenderComponent>([](const RenderComponent &lhs, const RenderComponent &rhs) {
        return lhs.Mode < rhs.Mode;
    });

    _trailShader.compile(trailVertexShader, trailFragmentShader);

    glGenBuffers(1, &VBO);

    // _physics->AddCube(0.0, glm::vec3(10, 10, 10), origin + glm::vec3(20, 0, 0));

    std::vector<glm::vec3> triangles;

    for (size_t f = 0; f < _bspAsset->_faces.size(); f++)
    {
        auto &face = _bspAsset->_faces[f];

        if (face.flags != 0)
        {
            continue;
        }

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

    _vertexArray = new VertexArray();
    mesh = _vertexArray->add(vertices, 36, glm::vec3(10.0f));
    _vertexArray->upload();

    _character = _physics->AddCharacter(150, 20, 32, origin);

    return true;
}

std::vector<PhysicsComponent> balls;

bool GenMapApp::Tick(
    std::chrono::nanoseconds time,
    const struct InputState &inputState)
{
    _physics->Step(time);

    const float speed = 5.0f;
    float timeStep = float(time.count()) / 1000000.0f;

    if (timeStep > 10)
    {
        if (IsKeyboardButtonPushed(inputState, KeyboardButtons::KeySpace))
        {
            //_skipClipping = !_skipClipping;

            auto comp = _physics->AddCube(10.0f, glm::vec3(10), _cam.Position());
            _physics->ApplyForce(comp, _cam.Forward() * 50050.0f);
            balls.push_back(comp);
        }

        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyLeft] || inputState.KeyboardButtonStates[KeyboardButtons::KeyA])
        {
            //_cam.MoveLeft(speed * timeStep);
            _physics->MoveCharacter(_character, _cam.Left(), speed * timeStep);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyRight] || inputState.KeyboardButtonStates[KeyboardButtons::KeyD])
        {
            //_cam.MoveLeft(-speed * timeStep);
            _physics->MoveCharacter(_character, -1.0f * _cam.Left(), speed * timeStep);
        }

        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyUp] || inputState.KeyboardButtonStates[KeyboardButtons::KeyW])
        {
            // _cam.MoveForward(speed * timeStep);
            _physics->MoveCharacter(_character, _cam.Forward(), speed * timeStep);
        }
        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyDown] || inputState.KeyboardButtonStates[KeyboardButtons::KeyS])
        {
            // _cam.MoveForward(-speed * timeStep);
            _physics->MoveCharacter(_character, -1.0f * _cam.Forward(), speed * timeStep);
        }

        //        if (inputState.KeyboardButtonStates[KeyboardButtons::KeyQ])
        //        {
        //            _cam.MoveUp(speed * timeStep);
        //        }
        //        else if (inputState.KeyboardButtonStates[KeyboardButtons::KeyZ])
        //        {
        //            _cam.MoveUp(-speed * timeStep);
        //        }

        static int lastPointerX = inputState.MousePointerPosition[0];
        static int lastPointerY = inputState.MousePointerPosition[1];

        int diffX = -(inputState.MousePointerPosition[0] - lastPointerX);
        int diffY = -(inputState.MousePointerPosition[1] - lastPointerY);

        lastPointerX = inputState.MousePointerPosition[0];
        lastPointerY = inputState.MousePointerPosition[1];

        if (inputState.MouseButtonStates[MouseButtons::LeftButton])
        {
            _cam.RotateZ(glm::radians(float(diffX) * 0.1f));
            _cam.RotateX(glm::radians(float(diffY) * 0.1f));
        }

        auto m = _physics->GetMatrix(_character);
        _cam.SetPosition(glm::vec3(m[3]) + glm::vec3(0.0f, 0.0f, 40.0f));
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSky();
    RenderBsp();
    RenderTrail();

    glDisable(GL_CULL_FACE);
    _trailShader.use();

    for (auto &ball : balls)
    {
        auto m = _physics->GetMatrix(ball);

        _trailShader.setupMatrices(_projectionMatrix * _cam.GetViewMatrix() * m);

        _vertexArray->render(VertexArrayRenderModes::Triangles, std::get<0>(mesh), std::get<1>(mesh));
    }
    glEnable(GL_CULL_FACE);

    VertexArray vertexAndColorBuffer;

    //_physics->RenderDebug(vertexAndColorBuffer);

    vertexAndColorBuffer.upload();

    glDisable(GL_DEPTH_TEST);
    vertexAndColorBuffer.render(VertexArrayRenderModes::Lines);
    glEnable(GL_DEPTH_TEST);

    return true; // to keep running
}

void GenMapApp::SetFilename(
    const char *root,
    const char *map)
{
    _fs.FindRootFromFilePath(root);
    _map = map;
}

void GenMapApp::SetupSky()
{
    // here we make up for the half of pixel to get the sky textures really stitched together because clamping is not enough
    const float uv_1 = 255.0f / 256.0f;
    const float uv_0 = 1.0f - uv_1;
    const float size = 1.0f;

    //if (renderFlag & SKY_BACK)
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

    //if (renderFlag & SKY_DOWN)
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

    //if (renderFlag & SKY_FRONT)
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

    //if (renderFlag & SKY_RIGHT)
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

    //if (renderFlag & SKY_UP)
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

    _skyShader.compile(skyVertexShader, skyFragmentShader);
    _skyVertexBuffer.setup(_skyShader);
}

void GenMapApp::SetupBsp()
{
    _normalBlendingShader.compileDefaultShader();
    _solidBlendingShader.compile(solidBlendingVertexShader, solidBlendingFragmentShader);

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
            ft.lightmapIndex = f;
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

    _vertexBuffer
        .setup(_normalBlendingShader);

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

        if (bspEntity.keyvalues.count("model") != 0 && bspEntity.classname.rfind("func_", 0) != 0)
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
                // todo, this probably is a mdl or spr file
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

    for (int i = 0; i < 6; i++)
    {
        _skyTextureIndices[i] = UploadToGl(_bspAsset->_skytextures[i]);
    }

    spdlog::debug("loaded {0} vertices", _vertexBuffer.vertexCount());
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

void GenMapApp::RenderTrail()
{
    //glDisable(GL_DEPTH_TEST);
    if (_trail.size() < 3)
    {
        return;
    }

    _trailShader.use();

    _trailShader.setupMatrices(_projectionMatrix * _cam.GetViewMatrix());

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, _trail.size() * sizeof(glm::vec3), _trail.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINE_LOOP, 0, _trail.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        shader.setupColor(glm::vec4(
            1.0f,
            1.0f,
            1.0f,
            1.0f));
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
            shader.setupColor(glm::vec4(
                1.0f,
                1.0f,
                1.0f,
                float(renderComponent.Amount) / 255.0f));
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
