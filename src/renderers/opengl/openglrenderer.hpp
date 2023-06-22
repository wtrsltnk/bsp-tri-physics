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

    virtual std::unique_ptr<IShader> LoadShader(
        const std::string &shaderName);

private:
    std::filesystem::path _assetFolder = std::filesystem::path("./assets");
};

#endif // OPENGLRENDERER_H
