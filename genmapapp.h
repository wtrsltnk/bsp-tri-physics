#ifndef GENMAPAPP_H
#define GENMAPAPP_H

#include "camera.h"
#include "entitycomponents.h"
#include "hl1bspasset.h"
#include "hl1filesystem.h"
#include "include/glbuffer.h"
#include "include/glshader.h"
#include "physicsservice.hpp"

#include <chrono>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glshader.h>
#include <map>
#include <string>
#include <vector>

class GlProgram
{
public:
    GlProgram()
    {
        _index = glCreateProgram();
    }

    ~GlProgram()
    {
        if (is_good())
        {
            glDeleteProgram(_index);
            _index = 0;
        }
    }

    template <int Type>
    void attach(const ShaderType &shader)
    {
        glAttachShader(_index, shader.id());
    }

    void link()
    {
        glLinkProgram(_index);

        GLint result = GL_FALSE;
        GLint logLength;

        glGetProgramiv(_index, GL_LINK_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetProgramiv(_index, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> error(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetProgramInfoLog(_index, logLength, NULL, &error[0]);

            std::cerr << "tried linking program, got error:\n"
                      << error.data();
        }
    }

    GLint getAttribLocation(const char *name) const { return glGetAttribLocation(_index, name); }

    void setUniformMatrix(const char *name, const glm::mat4 &m)
    {
        unsigned int model = glGetUniformLocation(_index, name);
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(m));
    }

    void use() const { glUseProgram(_index); }

    bool is_good() const { return _index > 0; }

private:
    GLuint _index = 0;
};

class FaceType
{
public:
    GLuint firstVertex;
    GLuint vertexCount;
    GLuint textureIndex;
    GLuint lightmapIndex;
    int flags;
};

class GenMapApp
{
public:
    enum SkyTextures
    {
        Back = 0,
        Down = 1,
        Front = 2,
        Left = 3,
        Right = 4,
        up = 5,
        Count,
    };

    void SetFilename(
        const char *root,
        const char *map);

    bool Startup();

    void SetupSky();

    void SetupBsp();

    void Resize(
        int width,
        int height);
    void Destroy();

    bool Tick(
        std::chrono::nanoseconds time,
        const struct InputState &inputState);

    void RenderTrail();

    void RenderSky();

    void RenderBsp();

    void RenderModelsByRenderMode(
        RenderModes mode,
        ShaderType &shader,
        const glm::mat4 &matrix);

private:
    FileSystem _fs;
    std::string _map;
    valve::hl1::BspAsset *_bspAsset = nullptr;
    VertexArray* _vertexArray;
    glm::mat4 _projectionMatrix = glm::mat4(1.0f);
    ShaderType _trailShader;
    ShaderType _skyShader;
    BufferType _skyVertexBuffer;
    GLuint _skyTextureIndices[6] = {0, 0, 0, 0, 0, 0};
    ShaderType _normalBlendingShader;
    ShaderType _solidBlendingShader;
    BufferType _vertexBuffer;
    std::vector<GLuint> _textureIndices;
    std::vector<GLuint> _lightmapIndices;
    std::vector<FaceType> _faces;
    std::map<GLuint, FaceType> _facesByLightmapAtlas;
    Camera _cam;
    entt::registry _registry;
    PhysicsService *_physics = nullptr;

    unsigned int VBO;
    std::vector<glm::vec3> _trail;
};

#endif // GENMAPAPP_H
