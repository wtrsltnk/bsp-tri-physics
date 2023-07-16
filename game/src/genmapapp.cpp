#include "genmapapp.h"

#include "assetmanager.h"
#include "physicsservice.hpp"
#include "renderers/opengl/openglrenderer.hpp"

#include <application.h>
#include <ctime>
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

bool showPhysicsDebug = false;

void EnableOpenGlDebug();

Game::Game()
{
    _fileSystem = std::make_unique<FileSystem>();
    _assets = std::make_unique<AssetManager>(_fileSystem.get());
}

void Game::SetFilename(
    const char *root,
    const char *map)
{
    _fileSystem->FindRootFromFilePath(root);

    if (map == nullptr)
    {
        _fileSystem->FindRootFromFilePath(root);
        _map = root;
    }
    else
    {
        _map = map;
    }
}

bool Game::Startup(
    void *windowHandle)
{
    spdlog::debug("Startup()");

    spdlog::info("{} @ {}", _fileSystem->Mod(), _fileSystem->Root().generic_string());

    _renderer = std::make_unique<OpenGlRenderer>();

    _physics = std::make_unique<PhysicsService>();

    _engine = std::make_unique<Engine>(_renderer.get(), _physics.get(), _assets.get());

    auto font = _fonts.LoadFont(L"consola", 12.0f);
    _console = std::make_unique<Console>(font);

    EnableOpenGlDebug();

    glClearColor(0.0f, 0.45f, 0.7f, 1.0f);

    _engine->Load(_map);

    return true;
}

bool Game::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    if (_console->Tick(time, inputState))
    {
        struct InputState emptyImputState = {};
        _engine->Update(time, emptyImputState);
    }
    else
    {
        _engine->Update(time, inputState);
    }

    //    if (IsMouseButtonPushed(inputState, MouseButtons::LeftButton))
    //    {
    //        const auto entity = _registry.create();

    //        auto comp = _physics->AddCube(1.0f, glm::vec3(30.0f), _cam.Position() + (_cam.Forward() * 40.0f));

    //        _physics->ApplyForce(comp, glm::normalize(_cam.Forward()) * 7000.0f);

    //        _registry.assign<PhysicsComponent>(entity, comp);
    //        _registry.assign<BallComponent>(entity, 0);
    //    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _engine->Render(time);

    //    auto cubes = _registry.view<PhysicsComponent, BallComponent>();

    //    glDisable(GL_BLEND);
    //    for (auto &entity : cubes)
    //    {
    //        auto physicsComponent = _registry.get<PhysicsComponent>(entity);

    //        auto m = glm::scale(_physics->GetMatrix(physicsComponent), glm::vec3(3.0f));

    //        _defaultShader->use();
    //        _defaultShader->setupSpriteType(9);
    //        _defaultShader->setupBrightness(0.5f);
    //        _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //        _defaultShader->setupMatrices(_projectionMatrix, _cam.GetViewMatrix(), m);

    //        glDrawArrays(GL_TRIANGLES, _firstCubeVertex, 36);
    //    }

    //    if (showPhysicsDebug)
    //    {
    //        VertexArray vertexAndColorBuffer;

    //        _physics->RenderDebug(vertexAndColorBuffer);

    //        vertexAndColorBuffer.upload();

    //        auto m = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 0.08f, 1.0f / 0.08f, 1.0f / 0.08f));

    //        _defaultShader->use();
    //        _defaultShader->setupSpriteType(9);
    //        _defaultShader->setupBrightness(0.5f);
    //        _defaultShader->setupColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //        _defaultShader->setupMatrices(_projectionMatrix, _cam.GetViewMatrix(), m);

    //        glActiveTexture(GL_TEXTURE1);
    //        glBindTexture(GL_TEXTURE_2D, _emptyWhiteTexture);

    //        glActiveTexture(GL_TEXTURE0);
    //        glBindTexture(GL_TEXTURE_2D, _emptyWhiteTexture);

    //        glDisable(GL_DEPTH_TEST);
    //        vertexAndColorBuffer.render(VertexArrayRenderModes::Lines);
    //        glEnable(GL_DEPTH_TEST);
    //    }

    _console->Render();

    return true; // to keep running
}

void Game::Resize(
    int width,
    int height)
{
    glViewport(0, 0, width, height);

    _engine->SetProjectionMatrix(
        glm::perspective(glm::radians(70.0f), float(width) / float(height), 0.1f, 4096.0f));

    _console->Resize(width, height);
}

void Game::Destroy()
{
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

//// pos x, y, z, color h, s, v
// float GenMapApp::vertices[216] = {
//     -0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     -0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     -0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,

//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,

//    0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//};
