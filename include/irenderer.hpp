#ifndef IRENDERER_H
#define IRENDERER_H

#include <glm/glm.hpp>
#include <memory>
#include <string>

class IShader
{
public:
    virtual ~IShader() {}

    virtual void use() const = 0;

    virtual void setupMatrices(
        const glm::mat4 &proj,
        const glm::mat4 &view,
        const glm::mat4 &model) = 0;

    virtual void setupColor(
        const glm::vec4 &color) = 0;

    virtual void setupBrightness(
        float brightness) = 0;

    virtual void setupSpriteType(
        int type) = 0;

    virtual void BindBones(
        const glm::mat4 m[],
        size_t count) = 0;

    virtual void UnbindBones() = 0;
};

class IRenderer
{
public:
    virtual ~IRenderer() {}

    virtual unsigned int LoadTexture(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data) = 0;

    virtual unsigned int LoadLightmap(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data) = 0;

    virtual std::unique_ptr<IShader> LoadShader(
        const std::string &shaderName) = 0;

    virtual void BindTexture(
        unsigned int index) = 0;

    virtual void BindLightmap(
        unsigned int index) = 0;

    virtual void EnableDepthTesting() = 0;

    virtual void DisableDepthTesting() = 0;
};

#endif // IRENDERER_H
