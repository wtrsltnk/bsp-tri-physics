#include "genmapapp.h"

#include "assetmanager.h"
#include "physicsservice.hpp"
#include "renderers/opengl/openglrenderer.hpp"

#include <application.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

bool showPhysicsDebug = false;

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

std::shared_ptr<IFont> font;

bool Game::Startup(
    void *windowHandle)
{
    spdlog::debug("Startup()");

    spdlog::info("{} @ {}", _fileSystem->Mod(), _fileSystem->Root().generic_string());

    _renderer = std::make_unique<OpenGlRenderer>();

    _physics = std::make_unique<PhysicsService>();

    _engine = std::make_unique<Engine>(_renderer.get(), _physics.get(), _assets.get());

    font = _fonts.LoadFont(L"consola", 12.0f);

    _console = std::make_unique<Console>(font);

    _engine->Load(_map);

    return true;
}

bool Game::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    int fps = 0;
    if (time.count() > 0)
    {
        fps = int(1000000.0 / double(time.count()));
    }

    std::wstringstream wss;
    wss << fps;
    font->Print(wss.str().c_str(), 10, 10);

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
    _renderer->Resize(width, height);

    _engine->SetProjectionMatrix(
        glm::perspective(glm::radians(70.0f), float(width) / float(height), 0.1f, 4096.0f));

    _console->Resize(width, height);
}

void Game::Destroy()
{
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
