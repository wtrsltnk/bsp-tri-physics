#include "openglrenderer.hpp"

#include <glad/glad.h>
#include <glshader.h>
#include <spdlog/spdlog.h>

OpenGlRenderer::~OpenGlRenderer() = default;

unsigned int OpenGlRenderer::LoadTexture(
    int width,
    int height,
    int bpp,
    bool repeat,
    unsigned char *data)
{
    GLuint format = GL_RGB;
    GLuint glIndex = 0;

    switch (bpp)
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

    if (repeat)
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

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    return glIndex;
}

std::unique_ptr<IShader> OpenGlRenderer::LoadShader(
    const std::string &shaderName)
{
    auto shader = std::make_unique<ShaderType>();

    if (shaderName.empty())
    {
        if (!shader->compileDefaultShader())
        {
            spdlog::error("failed to compile default shader");

            return nullptr;
        }
    }
    else
    {
        // todo: determine shader file names here...
        std::filesystem::path vertexShaderFile = _assetFolder / "shaders" / (shaderName + ".vert");
        std::filesystem::path fragmentShaderFile = _assetFolder / "shaders" / (shaderName + ".frag");

        shader->compile(vertexShaderFile.string(), fragmentShaderFile.string());
    }

    if (!shader->setupAttributes())
    {
        spdlog::error("failed to setup shader attributes");

        return nullptr;
    }

    return std::move(shader);
}
