#ifndef GENMAPAPP_H
#define GENMAPAPP_H

#include <engine.hpp>

#include "console.h"
#include <application.h>
#include <chrono>
#include <entt/entt.hpp>
#include <fontmanager.h>
#include <glbuffer.h>
#include <glm/glm.hpp>
#include <irenderer.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Game : public IApplication
{
public:
    Game();

    virtual void SetFilename(
        const char *root,
        const char *map);

    virtual bool Startup(
        void *windowHandle);

    virtual void Resize(
        int width,
        int height);

    virtual void Destroy();

    virtual bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    virtual bool RawEvent(
        void *)
    {
        return false;
    }

private:
    std::unique_ptr<IRenderer> _renderer;
    std::unique_ptr<IPhysicsService> _physics;
    std::unique_ptr<IAssetManager> _assets;
    std::unique_ptr<valve::IFileSystem> _fileSystem;
    std::unique_ptr<Engine> _engine;
    std::unique_ptr<Console> _console;
    FontManager _fonts;
    std::string _map;
};

#endif // GENMAPAPP_H
