#ifndef OPENGLRENDERER_H
#define OPENGLRENDERER_H

#include <filesystem>
#include <irenderer.hpp>

class OpenGlRenderer : public IRenderer
{
public:
    virtual ~OpenGlRenderer();

    virtual unsigned int LoadTexture(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data);

    virtual unsigned int LoadLightmap(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data);

    virtual std::unique_ptr<IShader> LoadShader(
        const std::string &shaderName);

    virtual void BindTexture(
        unsigned int index);

    virtual void BindLightmap(
        unsigned int index);

    virtual void EnableDepthTesting();

    virtual void DisableDepthTesting();

private:
    std::filesystem::path _assetFolder = std::filesystem::path("./assets");

    unsigned int LoadActualTexture(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data);
};

#endif // OPENGLRENDERER_H
