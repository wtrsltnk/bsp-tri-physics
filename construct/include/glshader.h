#ifndef GLSHADER_H
#define GLSHADER_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <irenderer.hpp>
#include <string>

#define GLSL(src) "#version 400\n" #src

class ShaderType : public IShader
{
public:
    ShaderType();

    virtual ~ShaderType();

    GLuint id() const;

    void use() const;

    GLuint compile(
        const std::string &vertShaderStr,
        const std::string &fragShaderStr,
        int boneCount = 0);

    GLuint compileDefaultShader();

    void setupMatrices(
        const glm::mat4 &proj,
        const glm::mat4 &view,
        const glm::mat4 &model);

    void setupColor(
        const glm::vec4 &color);

    void setupBrightness(
        float brightness);

    void setupSpriteType(
        int type);

    bool setupAttributes();

    void BindBones(
        const glm::mat4 m[],
        size_t count);

    void UnbindBones();

private:
    GLuint _shaderId = 0;
    GLuint _projUniformId = 0;
    GLuint _viewUniformId = 0;
    GLuint _modelUniformId = 0;
    GLuint _colorUniformId = 0;
    GLuint _u_spritetypeId = 0;
    GLuint _brightnessUniformId = 0;
    GLuint _bonesUniformId = 0;
    unsigned int _bonesBuffer = 0;
};

#endif // GLSHADER_H
