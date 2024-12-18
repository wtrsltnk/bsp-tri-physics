#include "openglrenderer.hpp"

#include <glad/glad.h>
#include <glshader.h>
#include <spdlog/spdlog.h>

OpenGlRenderer::~OpenGlRenderer() = default;

void EnableOpenGlDebug();

OpenGlRenderer::OpenGlRenderer()
{
    EnableOpenGlDebug();

    glClearColor(0.0f, 0.45f, 0.7f, 1.0f);
}

void OpenGlRenderer::Resize(
    int width,
    int height)
{
    glViewport(0, 0, width, height);
}

unsigned int OpenGlRenderer::LoadTexture(
    int width,
    int height,
    int bpp,
    bool repeat,
    unsigned char *data)
{
    glActiveTexture(GL_TEXTURE0);

    return LoadActualTexture(width, height, bpp, repeat, data);
}

unsigned int OpenGlRenderer::LoadLightmap(
    int width,
    int height,
    int bpp,
    bool repeat,
    unsigned char *data)
{
    glActiveTexture(GL_TEXTURE1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return LoadActualTexture(width, height, bpp, repeat, data);
}

unsigned int OpenGlRenderer::LoadActualTexture(
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

void OpenGlRenderer::BindTexture(
    unsigned int index)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, index);
}

void OpenGlRenderer::BindLightmap(
    unsigned int index)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, index);
}

void OpenGlRenderer::EnableDepthTesting()
{
    glEnable(GL_DEPTH_TEST);
}

void OpenGlRenderer::DisableDepthTesting()
{
    glDisable(GL_DEPTH_TEST);
}

void OpenGlRenderer::RenderTriangleFans(
    int start,
    int count)
{
    glDrawArrays(GL_TRIANGLE_FAN, start, count);
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
