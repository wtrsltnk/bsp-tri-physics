#ifndef GOLDSRCVIEWERAPP_H
#define GOLDSRCVIEWERAPP_H

#include <engine.hpp>

#include <application.h>
#include <chrono>
#include <entt/entt.hpp>
#include <glbuffer.h>
#include <glm/glm.hpp>
#include <irenderer.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

class GoldSrcViewerApp : public IApplication
{
public:
    GoldSrcViewerApp();

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
        void *eventPackage);

private:
    std::unique_ptr<IRenderer> _renderer;
    std::unique_ptr<IPhysicsService> _physics;
    std::unique_ptr<IAssetManager> _assets;
    std::unique_ptr<valve::IFileSystem> _fileSystem;
    std::unique_ptr<Engine> _engine;
    std::string _map;
};

#endif // GOLDSRCVIEWERAPP_H
